/* The Lisa platform layer (see lisagfx.h). Everything is polled with
   interrupts off: the boot block left the 68000 at priority 7.

   Hardware, as the boot ROM uses it and IDLE emulates it:
   - The screen is 32K at the logical address the ROM leaves at $110,
     90 bytes a row, the most significant bit leftmost, 1 = black.
   - The MMU status register ($FCF801) has the vertical retrace in bit 2:
     it goes to 0 when a retrace starts, if the video interrupt is enabled
     (a write to $FCE01A), and back to 1 when the interrupt is cleared (a
     write to $FCE018).
   - The keyboard and mouse talk to the COPS, which hands bytes to VIA1
     port A ($FCDD83); CA1 (bit 1 of the flag register at $FCDD9B) says a
     byte is waiting, and reading port A takes it. A key is its number
     with bit 7 set when it goes down; the mouse button is key $06, and
     $00 is followed by how far the mouse moved, dx then dy, signed. */
#include "lisagfx.h"
#include "lisafont.h"
#include "lisasound.h"

typedef unsigned char u8;

#define ROWB 90
#define STATUS (*(volatile u8 *)0xFCF801)
#define VID_OFF (*(volatile u8 *)0xFCE018)
#define VID_ON (*(volatile u8 *)0xFCE01A)
#define VIA1_ORA (*(volatile u8 *)0xFCDD83)
#define VIA1_IFR (*(volatile u8 *)0xFCDD9B)

/* Everything is drawn into scr, a buffer the size of the screen, and the
   rows changed (dirty_top to dirty_bot - 1) are copied to the screen at
   the next wait_vsync(), so a card is never seen half drawn. Rows are 90
   bytes and the buffers long-aligned, so the copy goes by longs. */
static unsigned long back[ROWB * GFX_H / 4 + 1];
static u8 *scr = (u8 *)back, *screen;
static short dirty_top = GFX_H, dirty_bot;
/* Where each row of the buffer starts: a look-up is much cheaper than a
   multiply on a 68000. And each bit of a byte doubled, for text two
   pixels wide. Both are filled in by gfx_init(). */
static u8 *rowp[GFX_H];
static unsigned short dbl[256];
static unsigned long frames;

static void mark(short y) {
    if (y < dirty_top) dirty_top = y;
    if (y >= dirty_bot) dirty_bot = (short)(y + 1);
}

static const u8 pats[5][8] = {
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /* white */
    {0x88, 0x22, 0x88, 0x22, 0x88, 0x22, 0x88, 0x22}, /* light grey */
    {0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55}, /* grey */
    {0x77, 0xDD, 0x77, 0xDD, 0x77, 0xDD, 0x77, 0xDD}, /* dark grey */
    {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, /* black */
};

/* ---- keyboard ---- */

/* Lisa key numbers (the COPS's code without the down bit) to what
   gfx_next_key() returns. From BMLisa's src/port/keymap.c (US layout). */
static const char keychar[128] = {
    [0x70] = 'a', [0x6E] = 'b', [0x6D] = 'c', [0x7B] = 'd', [0x60] = 'e',
    [0x69] = 'f', [0x6A] = 'g', [0x6B] = 'h', [0x53] = 'i', [0x54] = 'j',
    [0x55] = 'k', [0x59] = 'l', [0x58] = 'm', [0x6F] = 'n', [0x5F] = 'o',
    [0x44] = 'p', [0x75] = 'q', [0x65] = 'r', [0x76] = 's', [0x66] = 't',
    [0x52] = 'u', [0x6C] = 'v', [0x77] = 'w', [0x7A] = 'x', [0x67] = 'y',
    [0x79] = 'z',
    [0x74] = '1', [0x71] = '2', [0x72] = '3', [0x73] = '4', [0x64] = '5',
    [0x61] = '6', [0x62] = '7', [0x63] = '8', [0x50] = '9', [0x51] = '0',
    [0x48] = '\r', [0x5C] = ' ', [0x5D] = ',', [0x5E] = '.',
    [0x46] = 0x03, /* Enter */
    [0x2F] = 0x03, /* keypad Enter */
    [0x28] = 0x1C, /* keypad 4: left */
    [0x2A] = 0x1D, /* keypad 6: right */
    [0x25] = 0x1E, /* keypad 8: up */
    [0x2D] = 0x1F, /* keypad 2: down */
};

#define KEYQ 16
static char keyq[KEYQ];
static u8 kq_head, kq_tail;

/* What the next COPS byte is: 0 a key, 1 after $80 (a reset code or the
   start of a clock report), 2 and 3 a mouse report's dx and dy (after
   $00), 4 the rest of a clock report. */
static u8 cops_state, cops_left;
static signed char mouse_dx;

/* ---- the mouse ---- */

/* The pointer, at mx, my (its tip), shown from the first time the mouse
   moves. Clicks are queued with where they were. */
static short mx = GFX_W / 2, my = GFX_H / 2;
static u8 mouse_seen;

#define CLICKQ 8
static short clickq[CLICKQ][2];
static u8 cq_head, cq_tail;

static void mouse_move(short dx, short dy) {
    mx = (short)(mx + dx);
    my = (short)(my + dy);
    if (mx < 0) mx = 0;
    if (mx > GFX_W - 1) mx = GFX_W - 1;
    if (my < 0) my = 0;
    if (my > GFX_H - 1) my = GFX_H - 1;
    mouse_seen = 1;
}

static void mouse_click(void) {
    u8 next = (u8)((cq_tail + 1) % CLICKQ);
    if (next == cq_head) return;
    clickq[cq_tail][0] = mx;
    clickq[cq_tail][1] = my;
    cq_tail = next;
}

static void cops_byte(u8 b) {
    switch (cops_state) {
    case 0:
        if (b == 0x80) cops_state = 1;
        else if (b == 0x00) cops_state = 2;
        else if (b == 0x86) mouse_click(); /* the button, key $06, down */
        else if (b & 0x80) {
            char c = keychar[b & 0x7F];
            u8 next = (u8)((kq_tail + 1) % KEYQ);
            if (c && next != kq_head) { keyq[kq_tail] = c; kq_tail = next; }
        }
        break;
    case 1:
        if ((b & 0xF0) == 0xE0) { cops_state = 4; cops_left = 5; }
        else cops_state = 0;
        break;
    case 2: mouse_dx = (signed char)b; cops_state = 3; break;
    case 3: mouse_move(mouse_dx, (signed char)b); cops_state = 0; break;
    default:
        if (--cops_left == 0) cops_state = 0;
        break;
    }
}

static void poll_cops(void) {
    while (VIA1_IFR & 0x02) cops_byte(VIA1_ORA);
}

int gfx_next_key(void) {
    char c;
    poll_cops();
    if (kq_head == kq_tail) return 0;
    c = keyq[kq_head];
    kq_head = (u8)((kq_head + 1) % KEYQ);
    return c;
}

int gfx_next_click(short *x, short *y) {
    poll_cops();
    if (cq_head == cq_tail) return 0;
    *x = clickq[cq_head][0];
    *y = clickq[cq_head][1];
    cq_head = (u8)((cq_head + 1) % CLICKQ);
    return 1;
}

void gfx_flush_input(void) {
    poll_cops();
    kq_head = kq_tail;
    cq_head = cq_tail;
}

/* The arrow, 10 pixels wide and 11 tall (Lisa pixels are tall), as the
   Mac's: a black arrow with a white edge. Each row is a mask (where the
   arrow is) and the black part inside it, leftmost pixel in bit 15. It
   is drawn straight onto the screen, never into the drawing buffer: at
   each frame it comes off (the screen under it is put back from the
   buffer), the changed rows go to the screen, and it goes back on. */
#define ARROW_H 11
static const unsigned short arrow_mask[ARROW_H] = {
    0xC000, 0xE000, 0xF000, 0xF800, 0xFC00, 0xFE00,
    0xFF00, 0xFF80, 0xFE00, 0xEF00, 0x0780,
};
static const unsigned short arrow_black[ARROW_H] = {
    0x0000, 0x4000, 0x6000, 0x7000, 0x7800, 0x7C00,
    0x7E00, 0x7F00, 0x7C00, 0x4600, 0x0300,
};
static short shown_x = -1, shown_y;

static void arrow_restore(void) {
    short r, b;
    if (shown_x < 0) return;
    for (r = 0; r < ARROW_H && shown_y + r < GFX_H; r++) {
        short o = (short)((shown_y + r) * ROWB + (shown_x >> 3));
        for (b = 0; b < 3 && (shown_x >> 3) + b < ROWB; b++) screen[o + b] = scr[o + b];
    }
    shown_x = -1;
}

static void arrow_draw(void) {
    short r, b;
    if (!mouse_seen) return;
    for (r = 0; r < ARROW_H && my + r < GFX_H; r++) {
        unsigned long m = (unsigned long)arrow_mask[r] << (8 - (mx & 7));
        unsigned long k = (unsigned long)arrow_black[r] << (8 - (mx & 7));
        u8 *p = screen + (my + r) * ROWB + (mx >> 3);
        for (b = 0; b < 3 && (mx >> 3) + b < ROWB; b++) {
            u8 mb = (u8)(m >> (16 - 8 * b)), kb = (u8)(k >> (16 - 8 * b));
            p[b] = (u8)((p[b] & ~mb) | kb);
        }
    }
    shown_x = mx;
    shown_y = my;
}

/* ---- the frame clock ---- */

void gfx_init(void) {
    long i;
    screen = *(u8 **)0x110;
    for (i = 0; i < GFX_H; i++) rowp[i] = scr + i * ROWB;
    for (i = 0; i < 256; i++) {
        unsigned short d = 0;
        short k;
        for (k = 0; k < 8; k++)
            if (i & (0x80 >> k)) d |= (unsigned short)(0xC000 >> (2 * k));
        dbl[i] = d;
    }
    for (i = 0; i < ROWB * GFX_H; i += 4) *(unsigned long *)(screen + i) = 0;
    VID_ON = 0;
}

void wait_vsync(void) {
    long n;
    /* If no retrace ever shows (it always should), give up after about a
       frame's worth of polling rather than hang. */
    for (n = 0; n < 6000; n++) {
        poll_cops();
        if (!(STATUS & 0x04)) break;
    }
    VID_OFF = 0;
    VID_ON = 0;
    frames++;
    snd_tick();
    /* What was drawn since the last frame goes to the screen in one go,
       with the pointer taken off and put back around it. */
    if (shown_x >= 0 && (dirty_top < dirty_bot || shown_x != mx || shown_y != my))
        arrow_restore();
    if (dirty_top < dirty_bot) {
        unsigned long *d = (unsigned long *)(screen + dirty_top * ROWB);
        const unsigned long *s = (const unsigned long *)(scr + dirty_top * ROWB);
        short n4 = (short)(((dirty_bot - dirty_top) * ROWB + 3) / 4);
        while (n4--) *d++ = *s++;
        dirty_top = GFX_H;
        dirty_bot = 0;
    }
    if (shown_x < 0) arrow_draw();
}

unsigned long gfx_frames(void) {
    return frames;
}

/* ---- shapes ---- */

/* Pixels x0..x1-1 of row y: the pattern's row (mode 0) or inverted
   (mode 1). Everything is drawn with this, so it is written for speed:
   whole bytes in the middle go four at a time once they are even. */
static void span(short y, short x0, short x1, u8 pat, u8 mode) {
    u8 *row, *q, p, lm, rm;
    short b0, b1, n;
    if (y < 0 || y >= GFX_H) return;
    if (x0 < 0) x0 = 0;
    if (x1 > GFX_W) x1 = GFX_W;
    if (x0 >= x1) return;
    mark(y);
    row = rowp[y];
    b0 = x0 >> 3;
    b1 = (x1 - 1) >> 3;
    lm = (u8)(0xFF >> (x0 & 7));
    rm = (u8)(0xFF << (7 - ((x1 - 1) & 7)));
    if (mode) {
        if (b0 == b1) { row[b0] ^= (u8)(lm & rm); return; }
        row[b0] ^= lm;
        for (q = row + b0 + 1, n = (short)(b1 - b0 - 1); n > 0; n--) *q++ ^= 0xFF;
        row[b1] ^= rm;
        return;
    }
    p = pats[pat][y & 7];
    if (b0 == b1) {
        lm &= rm;
        row[b0] = (u8)((row[b0] & ~lm) | (p & lm));
        return;
    }
    row[b0] = (u8)((row[b0] & ~lm) | (p & lm));
    row[b1] = (u8)((row[b1] & ~rm) | (p & rm));
    q = row + b0 + 1;
    n = (short)(b1 - b0 - 1);
    if (n > 0 && ((unsigned long)q & 1)) { *q++ = p; n--; }
    if (n >= 4) {
        unsigned long pl = p * 0x01010101UL, *ql = (unsigned long *)q;
        short n4 = (short)(n >> 2);
        n &= 3;
        while (n4--) *ql++ = pl;
        q = (u8 *)ql;
    }
    while (n-- > 0) *q++ = p;
}

void gfx_rect(short x, short y, short w, short h, unsigned char pat) {
    short j;
    for (j = 0; j < h; j++) span(y + j, x, x + w, pat, 0);
}

void gfx_invert(short x, short y, short w, short h) {
    short j;
    for (j = 0; j < h; j++) span(y + j, x, x + w, 0, 1);
}

static unsigned short isqrt(unsigned long v) {
    unsigned long r = 0, bit = 1UL << 30;
    while (bit > v) bit >>= 2;
    while (bit) {
        if (v >= r + bit) { v -= r + bit; r = (r >> 1) + bit; }
        else r >>= 1;
        bit >>= 2;
    }
    return (unsigned short)r;
}

/* How far row j of a rounded rectangle h tall is indented at each side:
   the corner is a quarter ellipse rx across and ry down, measured at the
   middle of the row.
   Working that out takes a 32-bit divide and a square root a row, far
   too slow on a 5 MHz 68000 to do for every card drawn, so each corner
   size is worked out once and kept: the UI uses only a handful. */
#define CORNER_MAX 48
#define CORNERS 16
static struct { short rx, ry; u8 in[CORNER_MAX]; } corner[CORNERS];
static u8 corners_used, corner_next;

static const u8 *corner_table(short rx, short ry) {
    u8 i;
    short j;
    long t, d = 4L * ry * ry;
    for (i = 0; i < corners_used; i++)
        if (corner[i].rx == rx && corner[i].ry == ry) return corner[i].in;
    i = corner_next;
    corner_next = (u8)((corner_next + 1) % CORNERS);
    if (corners_used < CORNERS) corners_used++;
    corner[i].rx = rx;
    corner[i].ry = ry;
    for (j = 0; j < ry && j < CORNER_MAX; j++) {
        t = 2L * (ry - j) - 1; /* twice the distance from the corner's centre */
        corner[i].in[j] = (u8)(rx - isqrt((unsigned long)((long)rx * rx * (d - t * t) / d)));
    }
    return corner[i].in;
}

static short inset(short j, short h, short rx, short ry) {
    if (rx <= 0 || ry <= 0) return 0;
    if (ry > CORNER_MAX) ry = CORNER_MAX;
    if (j >= h - ry) j = (short)(h - 1 - j);
    if (j >= ry) return 0;
    return corner_table(rx, ry)[j];
}

void gfx_round4(short x, short y, short w, short h, short rx, short ry,
                unsigned char tl, unsigned char tr, unsigned char bl, unsigned char br) {
    short j, in, l, r, mid = x + w / 2;
    for (j = 0; j < h; j++) {
        unsigned char a = j < h / 2 ? tl : bl, b = j < h / 2 ? tr : br;
        in = inset(j, h, rx, ry);
        l = x + in;
        r = x + w - in;
        if (a == b) { span(y + j, l, r, a, 0); continue; }
        span(y + j, l, mid < r ? mid : r, a, 0);
        span(y + j, mid > l ? mid : l, r, b, 0);
    }
}

void gfx_round(short x, short y, short w, short h, short rx, short ry, unsigned char pat) {
    gfx_round4(x, y, w, h, rx, ry, pat, pat, pat, pat);
}

void gfx_round_invert(short x, short y, short w, short h, short rx, short ry) {
    short j, in;
    for (j = 0; j < h; j++) {
        in = inset(j, h, rx, ry);
        span(y + j, x + in, x + w - in, 0, 1);
    }
}

void gfx_round_frame(short x, short y, short w, short h, short rx, short ry, short t) {
    short tv = (short)((t * 2 + 1) / 3), j, in, iin;
    if (tv < 1) tv = 1;
    for (j = 0; j < h; j++) {
        in = inset(j, h, rx, ry);
        if (j < tv || j >= h - tv) { span(y + j, x + in, x + w - in, PAT_BLACK, 0); continue; }
        iin = (short)(t + inset((short)(j - tv), (short)(h - 2 * tv), (short)(rx - t), (short)(ry - tv)));
        if (iin < in + 1) iin = (short)(in + 1);
        span(y + j, x + in, x + iin, PAT_BLACK, 0);
        span(y + j, x + w - iin, x + w - in, PAT_BLACK, 0);
    }
}

/* ---- text ---- */


/* Set (or clear) the left-aligned bits of m in row y from pixel x. */
static void put_bits(short x, short y, unsigned long m, u8 white) {
    u8 *row;
    short b;
    if (y < 0 || y >= GFX_H || x < 0) return;
    mark(y);
    row = rowp[y];
    b = x >> 3;
    m >>= (x & 7);
    while (m && b < ROWB) {
        u8 v = (u8)(m >> 24);
        if (white) row[b] &= (u8)~v;
        else row[b] |= v;
        m <<= 8;
        b++;
    }
}

static short advance(unsigned char sx, unsigned char flags) {
    return (short)((flags & TXT_BOLD ? 7 : 6) * sx);
}

void gfx_text(short x, short y, const char *s, unsigned char sx, unsigned char sy,
              unsigned char flags) {
    for (; *s; s++, x += advance(sx, flags)) {
        unsigned char c = (unsigned char)*s, r, col, k, half, h = sx > 4 ? 3 : 6;
        const unsigned char *g;
        if (c < FONT_FIRST || c > FONT_LAST) c = '?';
        g = font5x9[c - FONT_FIRST];
        for (r = 0; r < FONT_ROWS; r++) {
            unsigned char bits = g[r];
            unsigned long m;
            if (!bits) continue;
            if (flags & TXT_BOLD) bits |= (unsigned char)(bits >> 1);
            if (sx <= 2) { /* nearly all text: one look-up a row */
                m = sx == 1 ? (unsigned long)bits << 24 : (unsigned long)dbl[bits] << 16;
                for (k = 0; k < sy; k++)
                    put_bits(x, (short)(y + r * sy + k), m, flags & TXT_WHITE);
                continue;
            }
            /* A row of 6 columns sx wide, shifted up to 7 pixels into its
               first byte, must fit put_bits()'s 32 bits: wider than sx 4,
               it goes in two halves of 3 columns. */
            for (half = 0; half < 6; half += h) {
                m = 0;
                for (col = 0; col < h; col++)
                    if (bits & (0x80 >> (half + col)))
                        m |= (((1UL << sx) - 1) << (32 - sx)) >> (col * sx);
                for (k = 0; k < sy; k++)
                    put_bits((short)(x + half * sx), (short)(y + r * sy + k), m, flags & TXT_WHITE);
            }
        }
    }
}

short gfx_text_w(const char *s, unsigned char sx, unsigned char flags) {
    short n = 0;
    while (s[n]) n++;
    return n ? (short)(n * advance(sx, flags) - sx) : 0;
}
