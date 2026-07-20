#include <tos.h>
#include "stevid.h"
#include "stefont.h"

/* card action-value codes (from cards.h; kept local so this file stays a
   pure video layer) */
#define VAL_SKIP 10
#define VAL_REVERSE 11
#define VAL_DRAW2 12
#define VAL_WILD 13
#define VAL_WILD4 14

static unsigned char *screen;          /* physical screen base */
static short old_rez;                  /* resolution to restore on exit */
static short old_pal[16];              /* palette to restore on exit */
static short pal[16];                  /* our working palette */
static long mch;                       /* _MCH cookie value */

/* ---- machine detection ---- */

/* Runs under Supexec: the cookie jar pointer at $5A0 is a protected system
   variable. _MCH high word: 0 = ST, 1 = STE/Mega STE, 2 = TT, 3 = Falcon. */
static long read_mch(void) {
    long *jar = *(long **)0x5A0L;
    mch = 0;
    if (jar) {
        while (jar[0]) {
            if (jar[0] == 0x5F4D4348L) {   /* '_MCH' */
                mch = jar[1];
                break;
            }
            jar += 2;
        }
    }
    return 0;
}

int ste_is_ste(void) {
    return ((mch >> 16) & 0xFFFF) == 1;
}

/* ---- setup ---- */

void ste_init(void) {
    int i;
    Supexec(read_mch);

    old_rez = Getrez();
    for (i = 0; i < 16; i++) old_pal[i] = 0;

    /* Low resolution (rez 0), keeping TOS's screen memory. */
    Setscreen((void *)-1L, (void *)-1L, 0);
    screen = (unsigned char *)Physbase();

    Cursconf(0, 0);                    /* hide the blinking text cursor */

    for (i = 0; i < 16; i++) pal[i] = 0;
    ste_clear(SC_BLACK);
}

void ste_shutdown(void) {
    Setscreen((void *)-1L, (void *)-1L, old_rez);
    Setpalette(old_pal);
    Cursconf(1, 0);
}

/* STE palette encoding: each gun is 4 bits, but the extra (4th) bit lives in
   bit 3 of the nibble as the LEAST significant bit, so a plain ST -- which
   only looks at bits 2-0 -- sees the top 3 bits and shows the nearest of its
   512 colours. One encoding, correct on both machines. */
static short ste_nibble(unsigned char v) {
    return (short)(((v >> 1) & 7) | ((v & 1) << 3));
}

void ste_palette(unsigned char idx, unsigned char r, unsigned char g, unsigned char b) {
    if (idx > 15) return;
    pal[idx] = (short)((ste_nibble(r) << 8) | (ste_nibble(g) << 4) | ste_nibble(b));
    Setpalette(pal);
}

void ste_wait_vsync(void) {
    Vsync();
}

/* ---- planar drawing ---- */

void ste_clear(unsigned char color) {
    ste_fill_rect(0, 0, BMP_W, BMP_H, color);
}

void ste_pixel(unsigned int x, unsigned int y, unsigned char color) {
    unsigned short *w;
    unsigned short mask;
    int p;
    if (x >= BMP_W || y >= BMP_H) return;
    w = (unsigned short *)(screen + y * BMP_STRIDE + ((x >> 4) * 8));
    mask = (unsigned short)(0x8000u >> (x & 15));
    for (p = 0; p < 4; p++) {
        if (color & (1 << p)) w[p] |= mask;
        else w[p] &= (unsigned short)~mask;
    }
}

void ste_hline(unsigned int x, unsigned int y, unsigned int w, unsigned char color) {
    ste_fill_rect(x, y, w, 1, color);
}

/* Word-masked solid fill: for each 16-pixel word group touched, build the
   affected-bit mask once and apply it to all four planes. Only the ragged
   left/right ends need partial masks, so wide fills (the card bodies) run at
   whole-word speed instead of per-pixel. */
void ste_fill_rect(unsigned int x, unsigned int y, unsigned int w, unsigned int h, unsigned char color) {
    unsigned int x2, g, g0, g1, row;
    unsigned char *rowp;

    if (w == 0 || h == 0 || x >= BMP_W || y >= BMP_H) return;
    if (x + w > BMP_W) w = BMP_W - x;
    if (y + h > BMP_H) h = BMP_H - y;
    x2 = x + w - 1;
    g0 = x >> 4;
    g1 = x2 >> 4;

    rowp = screen + y * BMP_STRIDE;
    for (row = 0; row < h; row++) {
        for (g = g0; g <= g1; g++) {
            unsigned short mask = 0xFFFFu;
            unsigned short *wp = (unsigned short *)(rowp + g * 8);
            int p;
            if (g == g0) mask &= (unsigned short)(0xFFFFu >> (x & 15));
            if (g == g1) mask &= (unsigned short)(0xFFFFu << (15 - (x2 & 15)));
            for (p = 0; p < 4; p++) {
                if (color & (1 << p)) wp[p] |= mask;
                else wp[p] &= (unsigned short)~mask;
            }
        }
        rowp += BMP_STRIDE;
    }
}

void ste_frame_rect(unsigned int x, unsigned int y, unsigned int w, unsigned int h, unsigned char color) {
    unsigned int row;
    if (w < 2 || h < 2) return;
    ste_hline(x, y, w, color);
    ste_hline(x, y + h - 1, w, color);
    for (row = 1; row < h - 1; row++) {
        ste_pixel(x, y + row, color);
        ste_pixel(x + w - 1, y + row, color);
    }
}

/* ---- text ---- */

void ste_char(unsigned int x, unsigned int y, unsigned char ch, unsigned char fg) {
    const unsigned char *glyph;
    unsigned int row;
    if (ch < FONT_FIRST || ch > FONT_LAST) ch = ' ';
    glyph = font8x8[ch - FONT_FIRST];
    for (row = 0; row < 8; row++) {
        unsigned char b = glyph[row];
        unsigned int col;
        if (!b) continue;
        for (col = 0; col < 8; col++)
            if (b & (0x80 >> col))
                ste_pixel(x + col, y + row, fg);
    }
}

void ste_text(unsigned int x, unsigned int y, const char *s, unsigned char fg) {
    while (*s) {
        ste_char(x, y, (unsigned char)*s++, fg);
        x += 8;
    }
}

/* ---- card art ---- */

/* suit (0-4) -> palette index */
static const unsigned char suit_col[5] = {SC_RED, SC_YELLOW, SC_GREEN, SC_BLUE, SC_GRAY};

#define CARD_W STE_CARD_W
#define CARD_H STE_CARD_H

static char value_char(unsigned char v) {
    if (v <= 9) return (char)('0' + v);
    switch (v) {
        case VAL_SKIP: return 'S';
        case VAL_REVERSE: return 'R';
        case VAL_DRAW2: return 'D';
        case VAL_WILD: return 'W';
        default: return 'F';
    }
}

void ste_card(unsigned int x, unsigned int y, unsigned char suit, unsigned char value) {
    unsigned char sc = suit_col[suit <= 4 ? suit : 4];
    char vc = value_char(value);

    /* Fill-based border (fast on planar): suit-coloured rect, then a white
       body inset by 3px leaves the border showing. */
    ste_fill_rect(x + 3, y + 3, CARD_W, CARD_H, SC_SHADOW);       /* drop shadow */
    ste_fill_rect(x, y, CARD_W, CARD_H, sc);                      /* suit border */
    ste_fill_rect(x + 3, y + 3, CARD_W - 6, CARD_H - 6, SC_WHITE);/* body */
    ste_char(x + 4, y + 4, (unsigned char)vc, sc);               /* top-left value */
    ste_char(x + CARD_W - 12, y + CARD_H - 12, (unsigned char)vc, sc); /* bottom-right */
    ste_fill_rect(x + CARD_W / 2 - 8, y + CARD_H / 2 - 9, 16, 18, sc); /* centre badge */
    ste_char(x + CARD_W / 2 - 4, y + CARD_H / 2 - 4, (unsigned char)vc, SC_WHITE);
}

void ste_card_back(unsigned int x, unsigned int y) {
    ste_fill_rect(x + 3, y + 3, CARD_W, CARD_H, SC_SHADOW);
    ste_fill_rect(x, y, CARD_W, CARD_H, SC_WHITE);
    ste_fill_rect(x + 3, y + 3, CARD_W - 6, CARD_H - 6, SC_RED);
    ste_fill_rect(x + CARD_W / 2 - 7, y + CARD_H / 2 - 6, 14, 12, SC_WHITE);
    ste_char(x + CARD_W / 2 - 4, y + CARD_H / 2 - 4, 'U', SC_RED);
}

#define HAND_LIFT 10

/* Overlap step: cards fan left-to-right showing each one's left corner-value
   strip. Wider for small hands, tighter for big ones so a 20-card hand fits. */
static int hand_step(unsigned char count) {
    int s;
    if (count <= 1) return 0;
    s = (304 - CARD_W) / (count - 1);
    if (s > 20) s = 20;
    if (s < 10) s = 10;
    return s;
}

void ste_hand(unsigned int hx, unsigned int hy, const unsigned char *suits,
              const unsigned char *values, unsigned char count, unsigned char selected) {
    unsigned char i;
    int step = hand_step(count);
    unsigned int sx;

    for (i = 0; i < count; i++) {
        if (i == selected) continue;
        ste_card(hx + (unsigned int)(i * step), hy, suits[i], values[i]);
    }

    sx = hx + (unsigned int)(selected * step);
    ste_card(sx, hy - HAND_LIFT, suits[selected], values[selected]);
    ste_frame_rect(sx - 2, hy - HAND_LIFT - 2, CARD_W + 4, CARD_H + 4, SC_YELLOW);
    ste_frame_rect(sx - 1, hy - HAND_LIFT - 1, CARD_W + 2, CARD_H + 2, SC_YELLOW);
}
