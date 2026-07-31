#include <vdp.h>
#include "tivid.h"

/* The alphabet that exists in every coloured range. 23 glyphs, so a range is
   three colour groups (24 codes) with one spare. Everything a card face is
   built from: values 0-9 and the action letters, the four suit letters, the
   bracket pair, '?' for an unchosen wild, and a space for padding inside a
   tile (a space from the normal font would punch a hole of backdrop colour
   through the middle of a card). Order is arbitrary; glyph_slot[] indexes it. */
static const char CGLYPHS[] = "0123456789SVDWFRYGB[]? ";
#define NGLYPH 23

/* First character code of each colour range, indexed by COL_*. Entry 0
   (COL_NORMAL) is unused -- normal text is drawn from the ASCII font
   directly rather than from a range. */
static const unsigned char range_base[7] = {
    0,      /* COL_NORMAL   - not a range */
    128,    /* COL_RED      */
    152,    /* COL_YELLOW   */
    176,    /* COL_GREEN    */
    200,    /* COL_BLUE     */
    224,    /* COL_WILD     */
    0       /* COL_SELECTED */
};

/* (fg<<4)|bg for the three colour-table entries each range occupies. Card
   faces want a dark glyph on a bright suit colour; wild is the exception
   (white on magenta reads better than black does). */
static const unsigned char range_color[7] = {
    (COLOR_WHITE << 4) | COLOR_TRANS,      /* normal text: backdrop shows through */
    (COLOR_BLACK << 4) | COLOR_MEDRED,
    (COLOR_BLACK << 4) | COLOR_LTYELLOW,
    (COLOR_BLACK << 4) | COLOR_MEDGREEN,
    (COLOR_BLACK << 4) | COLOR_LTBLUE,
    (COLOR_WHITE << 4) | COLOR_MAGENTA,
    (COLOR_BLACK << 4) | COLOR_WHITE       /* cursor highlight */
};

/* ASCII -> index into CGLYPHS, or 0xFF. Built once at init so drawing a card
   is a table lookup instead of a 23-entry scan per cell; this machine reads
   its expansion RAM over a slow multiplexed 8-bit bus and a full redraw
   touches a few hundred cells. */
static unsigned char glyph_slot[128];

static void build_glyph_table(void) {
    unsigned char i;
    for (i = 0; i < 128; i++) glyph_slot[i] = 0xFF;
    for (i = 0; i < NGLYPH; i++) glyph_slot[(unsigned char)CGLYPHS[i]] = i;
}

/* Copy the pattern of each CGLYPHS entry out of the freshly loaded console
   font and into every coloured range, so each range is that same little
   alphabet rendered in its own colour. */
static void build_color_ranges(void) {
    unsigned char buf[8];
    unsigned char col, i;

    for (col = COL_RED; col <= COL_SELECTED; col++) {
        unsigned int base = range_base[col];
        for (i = 0; i < NGLYPH; i++) {
            vdpmemread(gPattern + ((unsigned int)(unsigned char)CGLYPHS[i] << 3), buf, 8);
            vdpmemcpy(gPattern + ((base + i) << 3), buf, 8);
        }
    }
}

static void set_color_table(void) {
    unsigned char group;
    unsigned char col;

    /* Default everything to the plain-text pair, then stamp each range over
       the three groups it owns. */
    for (group = 0; group < 32; group++) {
        vdpchar(gColor + group, range_color[COL_NORMAL]);
    }
    for (col = COL_RED; col <= COL_SELECTED; col++) {
        group = (unsigned char)(range_base[col] >> 3);
        vdpchar(gColor + group, range_color[col]);
        vdpchar(gColor + group + 1, range_color[col]);
        vdpchar(gColor + group + 2, range_color[col]);
    }
}

void ti_init(void) {
    set_graphics(VDP_SPR_8x8);
    charset();                  /* console ASCII font into codes 32-126 */
    build_glyph_table();
    build_color_ranges();
    set_color_table();
    /* Backdrop (and border) behind the transparent background of normal
       text. Dark blue to match the other ports rather than the TI's cyan. */
    VDP_SET_REGISTER(VDP_REG_COL, COLOR_DKBLUE);
    scr_clear();
}

void wait_vsync(void) {
    vdpwaitvint();
}

void scr_clear(void) {
    vdpmemset(gImage, ' ', COLS * ROWS);
}

/* Maps a character to the code that draws it in the requested colour.
   Anything outside the coloured alphabet falls back to plain ASCII, which
   just means it comes out white -- a visible but harmless degradation
   rather than a wrong glyph. */
static unsigned char code_for(char ch, unsigned char color) {
    unsigned char c = (unsigned char)ch;
    unsigned char slot;
    if (color == COL_NORMAL || c >= 128) return c;
    slot = glyph_slot[c];
    if (slot == 0xFF) return c;
    return (unsigned char)(range_base[color] + slot);
}

void scr_put(unsigned char x, unsigned char y, char ch, unsigned char color) {
    vdpchar(gImage + ((unsigned int)y << 5) + x, code_for(ch, color));
}

void scr_puts(unsigned char x, unsigned char y, const char *s, unsigned char color) {
    unsigned int addr = gImage + ((unsigned int)y << 5) + x;
    while (*s) {
        vdpchar(addr++, code_for(*s++, color));
    }
}

/* Right-pads to three columns so a shrinking count can't leave a stale digit
   behind (hands and pile counts both go two digits and back). */
void scr_put_num(unsigned char x, unsigned char y, unsigned int n) {
    unsigned int addr = gImage + ((unsigned int)y << 5) + x;
    if (n >= 100) {
        vdpchar(addr++, '0' + (n / 100));
        vdpchar(addr++, '0' + ((n / 10) % 10));
        vdpchar(addr, '0' + (n % 10));
    } else if (n >= 10) {
        vdpchar(addr++, '0' + (n / 10));
        vdpchar(addr++, '0' + (n % 10));
        vdpchar(addr, ' ');
    } else {
        vdpchar(addr++, '0' + n);
        vdpchar(addr++, ' ');
        vdpchar(addr, ' ');
    }
}

void scr_fill_rect(unsigned char x, unsigned char y, unsigned char w, unsigned char h) {
    unsigned char row;
    for (row = 0; row < h; row++) {
        vdpmemset(gImage + ((unsigned int)(y + row) << 5) + x, ' ', w);
    }
}
