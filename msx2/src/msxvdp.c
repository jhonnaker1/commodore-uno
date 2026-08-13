#include "msxvdp.h"
#include "cards.h"

__sfr __at 0x98 VDP_DATA;   /* VRAM read/write */
__sfr __at 0x99 VDP_ADDR;   /* write: address/register latch; read: status */
__sfr __at 0x9A VDP_PAL;    /* palette data */
__sfr __at 0x9B VDP_REGI;   /* indirect register write, see vdp_cmd() */

extern void bios_chgmod(unsigned char mode);

/* The BIOS interrupt handler bumps this every VBLANK. Using it rather than
   polling the VDP's own interrupt flag keeps us out of the BIOS's way --
   it owns that flag (see vdp_idle) and clearing it behind its back
   would cost the machine its keyboard. */
#define JIFFY (*(volatile unsigned int *)0xFC9E)

/* CGTBL: a pointer, in the BIOS ROM at 0x0004, to the machine's 8x8 font.
   Every MSX has one and it is always mapped in page 0, so the cartridge
   carries no font of its own -- and the game comes out in the same
   typeface as the rest of the machine. */
static const unsigned char *font;

/* ---- VDP plumbing -------------------------------------------------- */

/* Plain di/ei rather than SDCC's __critical, which is not safe here.
   __critical compiles to the save-and-restore form:

       ld a,i / di / push af / ...body... / pop af / ret po / ei

   `ld a,i` copies IFF2 into the P/V flag -- and has a documented Z80
   erratum: if a maskable interrupt is accepted during that very
   instruction, P/V comes back 0 whatever IFF2 really held. The restore
   then reads "interrupts were already off", skips the ei, and they stay
   off permanently: JIFFY stops counting and wait_vsync() spins for good.
   With a VBLANK every frame and thousands of these per screen redraw,
   landing in that one-instruction window is a matter of when, not if.

   Restoring the previous state buys nothing here anyway. Interrupts are
   enabled for the whole life of the program -- the BIOS handler is what
   drives JIFFY and fills the keyboard buffer -- and none of this is
   reachable from an interrupt handler, so the state to return to is
   always "on". */
#define IRQ_OFF() __asm di __endasm
#define IRQ_ON()  __asm ei __endasm

/* Both bytes of a register write go to the same port back to back. An
   interrupt landing between them -- and the BIOS takes one every frame --
   would leave the VDP's latch half-set, and the handler's own VDP access
   would then be read as the second half of ours. */
static void vdp_reg(unsigned char reg, unsigned char val) {
    IRQ_OFF();
    VDP_ADDR = val;
    VDP_ADDR = 0x80 | reg;
    IRQ_ON();
}

static void vdp_palette(unsigned char idx, unsigned char r, unsigned char g,
                        unsigned char b) {
    vdp_reg(16, idx);        /* palette pointer, auto-increments per entry */
    VDP_PAL = (r << 4) | b;  /* R and B share a byte, 3 bits each */
    VDP_PAL = g;
}

/* Point the VRAM address counter at `addr` for writing.
   R#14 holds address bits 14-16; it auto-increments when the counter runs
   past a 16K boundary, so a single run of writes can cross one.

   Callers must have waited for the command engine first -- see the note on
   vdp_idle() below. */
static void vdp_setwrite(unsigned int addr) {
    vdp_reg(14, (unsigned char)(addr >> 14) & 0x07);
    IRQ_OFF();
    VDP_ADDR = addr & 0xFF;
    VDP_ADDR = 0x40 | ((addr >> 8) & 0x3F);
    IRQ_ON();
}

/* Block until the command engine is idle. S#2 bit 0 (CE) is set while it
   is running.

   Two different things need this, and missing the second is the subtle one:

     - a new command must not be issued while one is still running; and
     - neither must a CPU access to VRAM. The blitter and the CPU reach
       VRAM through the same port, and a command still in flight will
       happily overwrite whatever the CPU has just written behind it.
       gfx_fill_rect() returns as soon as the command is *started*, so a
       full-screen clear is still painting for tens of milliseconds after
       it returns -- long enough to swallow the next few lines of text
       whole.

   R#15 selects which status register port 0x99 reads back, and the BIOS
   interrupt handler reads S#0 every frame to acknowledge the VDP's
   interrupt. Leaving R#15 pointing at S#2 would make the handler read the
   wrong register, the interrupt would never be acknowledged, and the
   machine would wedge. So: select, read, put it straight back, all with
   interrupts off so the handler cannot land in the middle. */
static void vdp_idle(void) {
    unsigned char s;
    for (;;) {
        IRQ_OFF();
        VDP_ADDR = 2;
        VDP_ADDR = 0x80 | 15;
        s = VDP_ADDR;
        VDP_ADDR = 0;
        VDP_ADDR = 0x80 | 15;
        IRQ_ON();
        if (!(s & 0x01)) return;
    }
}

/* Load the command engine's 15-register block and fire it.
   R#17 is the indirect-register pointer: aim it at R#32 and each write to
   port 0x9B lands in the next register along, so the block streams out
   without 15 separate two-byte register writes. Writing R#46 (the command
   itself) is what starts the operation.

   All coordinates are pixels. The V9938 manual calls the high-speed
   commands "byte" operations, which is about the engine's internal
   granularity, not the units you feed it -- what it costs you is that
   HMMV/HMMM cannot start or stop on an odd pixel. */
static void vdp_cmd(unsigned int sx, unsigned int sy,
                    unsigned int dx, unsigned int dy,
                    unsigned int nx, unsigned int ny,
                    unsigned char clr, unsigned char arg, unsigned char cmd) {
    vdp_idle();
    vdp_reg(17, 32);
    VDP_REGI = sx & 0xFF; VDP_REGI = sx >> 8;   /* R#32/33 */
    VDP_REGI = sy & 0xFF; VDP_REGI = sy >> 8;   /* R#34/35 */
    VDP_REGI = dx & 0xFF; VDP_REGI = dx >> 8;   /* R#36/37 */
    VDP_REGI = dy & 0xFF; VDP_REGI = dy >> 8;   /* R#38/39 */
    VDP_REGI = nx & 0xFF; VDP_REGI = nx >> 8;   /* R#40/41 */
    VDP_REGI = ny & 0xFF; VDP_REGI = ny >> 8;   /* R#42/43 */
    VDP_REGI = clr;                             /* R#44 */
    VDP_REGI = arg;                             /* R#45 */
    VDP_REGI = cmd;                             /* R#46 -- starts it */
}

#define CMD_HMMV 0xC0   /* fill a rectangle with R#44 */

/* ---- public surface ------------------------------------------------ */

void wait_vsync(void) {
    unsigned int t = JIFFY;
    while (JIFFY == t) { }
}

void gfx_fill_rect(int x, int y, int w, int h, unsigned char color) {
    if (w <= 0 || h <= 0) return;
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x >= GFX_W || y >= GFX_H || w <= 0 || h <= 0) return;

    /* Snap to byte boundaries: x down, w up. HMMV fills whole bytes and one
       byte is two pixels, so an odd edge cannot be expressed -- but the way
       it fails matters. A width of 1 truncates to zero bytes, and the V9938
       reads NX == 0 not as "fill nothing" but as "fill the whole width", so
       a one-pixel line asked for naively comes back as a band right across
       the screen. Rounding up here is what stops gfx_frame_rect's vertical
       edges from doing exactly that. */
    if (x & 1) { x--; w++; }
    if (w & 1) w++;

    if (x + w > GFX_W) w = GFX_W - x;
    if (y + h > GFX_H) h = GFX_H - y;
    /* R#44 carries two pixels, since the engine fills a byte at a time. */
    vdp_cmd(0, 0, (unsigned int)x, (unsigned int)y,
            (unsigned int)w, (unsigned int)h,
            (unsigned char)((color << 4) | color), 0, CMD_HMMV);
}

void gfx_clear(unsigned char color) {
    gfx_fill_rect(0, 0, GFX_W, GFX_H, color);
}

/* The vertical edges come out two pixels wide rather than one, since
   gfx_fill_rect rounds a width up to a whole byte. At this resolution that
   reads as a slightly heavier frame, which is no bad thing for a cursor. */
void gfx_frame_rect(int x, int y, int w, int h, unsigned char color) {
    gfx_fill_rect(x, y, w, 1, color);
    gfx_fill_rect(x, y + h - 1, w, 1, color);
    gfx_fill_rect(x, y, 1, h, color);
    gfx_fill_rect(x + w - 1, y, 1, h, color);
}

/* One font row is 8 pixels; in SCREEN 5 that is 4 bytes of two pixels each.
   `pair` turns a 2-bit slice of the row into one such byte, so a glyph row
   is four table lookups rather than eight per-pixel decisions. */
static void expand_row(const unsigned char *pair, unsigned char b) {
    VDP_DATA = pair[b >> 6];
    VDP_DATA = pair[(b >> 4) & 3];
    VDP_DATA = pair[(b >> 2) & 3];
    VDP_DATA = pair[b & 3];
}

static void make_pair(unsigned char *pair, unsigned char fg, unsigned char bg) {
    pair[0] = (bg << 4) | bg;
    pair[1] = (bg << 4) | fg;
    pair[2] = (fg << 4) | bg;
    pair[3] = (fg << 4) | fg;
}

void gfx_char(int x, int y, char ch, unsigned char fg, unsigned char bg) {
    unsigned char pair[4];
    const unsigned char *gl = font + (unsigned int)(unsigned char)ch * 8;
    unsigned char row;
    vdp_idle();
    make_pair(pair, fg, bg);
    for (row = 0; row < 8; row++) {
        vdp_setwrite(((unsigned int)(y + row) << 7) + ((unsigned int)x >> 1));
        expand_row(pair, gl[row]);
    }
}

/* Drawn a pixel row at a time across the whole string rather than a glyph
   at a time: characters are adjacent in VRAM, so one address setup covers
   the entire row and the VDP's auto-increment carries it along. That turns
   8 address setups per character into 8 for the whole line.

   `fnt` caches the global deliberately. Reading `font` directly inside the
   loop makes SDCC 4.6 emit `ld iy,#_font` to reach it while IY is already
   holding the string cursor -- it clobbers the cursor, saves and restores
   the clobbered value around the call, then increments that instead of the
   string pointer. The loop then never reaches the NUL and walks off through
   memory. Indexing with `i` rather than a second pointer keeps the cursor
   out of IY in the first place; the cached base keeps the global out of it.
   Both together are what make this safe -- see README.md. */
void gfx_text(int x, int y, const char *s, unsigned char fg, unsigned char bg) {
    const unsigned char *fnt = font;
    unsigned char pair[4];
    unsigned char row;
    vdp_idle();
    make_pair(pair, fg, bg);
    for (row = 0; row < 8; row++) {
        unsigned char i;
        vdp_setwrite(((unsigned int)(y + row) << 7) + ((unsigned int)x >> 1));
        for (i = 0; s[i]; i++)
            expand_row(pair, fnt[(unsigned int)(unsigned char)s[i] * 8 + row]);
    }
}

/* ---- card art ------------------------------------------------------ */

static const unsigned char suit_col[5] = {
    GC_RED, GC_YELLOW, GC_GREEN, GC_BLUE, GC_GREY
};

static char value_char(unsigned char value) {
    switch (value) {
    case VAL_SKIP:    return 'S';
    case VAL_REVERSE: return 'R';
    case VAL_DRAW2:   return 'D';
    case VAL_WILD:    return 'W';
    case VAL_WILD4:   return 'F';
    default:          return (char)('0' + value);
    }
}

void gfx_card(int x, int y, unsigned char suit, unsigned char value) {
    unsigned char sc = suit_col[suit <= 4 ? suit : 4];
    char vc = value_char(value);

    gfx_fill_rect(x + 2, y + 2, GFX_CARD_W, GFX_CARD_H, GC_SHADOW);
    gfx_fill_rect(x, y, GFX_CARD_W, GFX_CARD_H, sc);              /* border */
    gfx_fill_rect(x + 2, y + 2, GFX_CARD_W - 4, GFX_CARD_H - 4, GC_WHITE);
    gfx_char(x + 2, y + 2, vc, sc, GC_WHITE);                     /* corner */
    gfx_char(x + GFX_CARD_W - 10, y + GFX_CARD_H - 10, vc, sc, GC_WHITE);
    /* centre badge, in the suit colour with the value knocked out white */
    gfx_fill_rect(x + GFX_CARD_W / 2 - 6, y + GFX_CARD_H / 2 - 7, 12, 14, sc);
    gfx_char(x + GFX_CARD_W / 2 - 4, y + GFX_CARD_H / 2 - 4, vc, GC_WHITE, sc);
}

void gfx_card_back(int x, int y) {
    gfx_fill_rect(x + 2, y + 2, GFX_CARD_W, GFX_CARD_H, GC_SHADOW);
    gfx_fill_rect(x, y, GFX_CARD_W, GFX_CARD_H, GC_WHITE);
    gfx_fill_rect(x + 2, y + 2, GFX_CARD_W - 4, GFX_CARD_H - 4, GC_RED);
    gfx_fill_rect(x + GFX_CARD_W / 2 - 6, y + GFX_CARD_H / 2 - 6, 12, 12, GC_WHITE);
    gfx_char(x + GFX_CARD_W / 2 - 4, y + GFX_CARD_H / 2 - 4, 'U', GC_RED, GC_WHITE);
}

#define HAND_LIFT 8

static int hand_step(unsigned char count) {
    int s;
    if (count <= 1) return 0;
    s = (244 - GFX_CARD_W) / (count - 1);
    if (s > 20) s = 20;
    if (s < 8) s = 8;
    return s;
}

void gfx_hand(int hx, int hy, const unsigned char *suits,
              const unsigned char *values, unsigned char count,
              unsigned char selected) {
    unsigned char i;
    int step = hand_step(count);
    int sx;

    /* Drawn left to right so each card overlaps the one before; the
       selected card is skipped here and drawn last, lifted, so it is not
       clipped by its neighbour. */
    for (i = 0; i < count; i++) {
        if (i == selected) continue;
        gfx_card(hx + (int)i * step, hy, suits[i], values[i]);
    }
    sx = hx + (int)selected * step;
    gfx_card(sx, hy - HAND_LIFT, suits[selected], values[selected]);
    gfx_frame_rect(sx - 2, hy - HAND_LIFT - 2, GFX_CARD_W + 4, GFX_CARD_H + 4, GC_YELLOW);
    gfx_frame_rect(sx - 1, hy - HAND_LIFT - 1, GFX_CARD_W + 2, GFX_CARD_H + 2, GC_YELLOW);
}

/* ---- setup --------------------------------------------------------- */

void gfx_init(void) {
    font = *(const unsigned char *const *)0x0004;   /* CGTBL */

    /* CHGMOD rather than hand-poked mode registers: it sets the VRAM table
       bases and the mode bits correctly across every MSX2 variant, and
       getting R#2-R#5 wrong by hand is a silent, hard-to-see failure. */
    bios_chgmod(5);

    vdp_palette(GC_WHITE,  7, 7, 7);
    vdp_palette(GC_RED,    7, 1, 1);
    vdp_palette(GC_YELLOW, 7, 6, 0);
    vdp_palette(GC_GREEN,  0, 6, 1);
    vdp_palette(GC_BLUE,   1, 3, 7);
    vdp_palette(GC_GREY,   4, 4, 4);
    vdp_palette(GC_SHADOW, 0, 1, 0);
    vdp_palette(GC_FELT,   0, 3, 0);
    vdp_palette(GC_BLACK,  0, 0, 0);

    /* R#7's low nibble is the backdrop, which paints the border around the
       212-line active area -- left alone it keeps whatever the boot screen
       used and frames the table in an unrelated colour. */
    vdp_reg(7, GC_FELT);

    gfx_clear(GC_FELT);
}
