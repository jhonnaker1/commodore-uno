#include <cx16.h>
#include <cbm.h>
#include "x16vera.h"

/* ---- VERA access -------------------------------------------------------

   VERA is reached through the register block at $9F20 (cc65 exposes it as
   the `VERA` struct: .address / .address_hi / .data0 / .control). To touch
   VRAM you point the address port at a VRAM address, pick an auto-increment
   step, then read/write the data port. address is 17 bits: the low 16 in
   .address, bit 16 in .address_hi bit0; .address_hi bits 4-7 hold the
   increment code (1 = +1 per access). */

#define VROM_TEXT   0x1B000UL   /* KERNAL text map: 128 cells wide, cell = char,color */
#define VROM_PAL    0x1FA00UL   /* palette: 256 entries x 2 bytes */
#define VROM_SPRATT 0x1FC00UL   /* sprite attributes: 128 x 8 bytes */
#define VROM_SPRIMG 0x13000UL   /* free VRAM for our card sprite image */

#define MAP_STRIDE 128          /* cells per text-map row */
#define DC_VIDEO (*(volatile unsigned char *)0x9F29)

#define SPR_INK  254            /* sprite palette slot recolored per suit */
#define SPR_EDGE 253            /* sprite border (white) */

static void vera_seek(unsigned long addr, unsigned char inc) {
    VERA.control = 0;                                   /* ADDRSEL=0 -> data0 */
    VERA.address = (unsigned int)(addr & 0xFFFF);
    VERA.address_hi = (unsigned char)(((addr >> 16) & 1) | (inc << 4));
}

/* One VERA palette entry: 2 bytes, byte0 = (Green<<4)|Blue, byte1 = Red,
   each nibble 0-15. */
static void set_palette(unsigned char idx, unsigned char r, unsigned char g, unsigned char b) {
    vera_seek(VROM_PAL + (unsigned long)idx * 2, 1);
    VERA.data0 = (unsigned char)((g << 4) | b);
    VERA.data0 = r;
}

/* Logical color -> VERA cell color byte (background<<4 | foreground),
   palette indices per vera_init()'s palette. */
static const unsigned char cell_color[NUM_LOGICAL] = {
    0x00, /* COL_BLACK   */
    0x01, /* COL_WHITE   */
    0x02, /* COL_RED     */
    0x07, /* COL_YELLOW  */
    0x05, /* COL_GREEN   */
    0x06, /* COL_BLUE    */
    0x0E, /* COL_CYAN    (light blue stand-in) */
    0x08, /* COL_MAGENTA (orange stand-in)     */
    0x08, /* COL_ORANGE  */
    0x21, /* TILE_RED:      white on red     */
    0x70, /* TILE_YELLOW:   black on yellow  */
    0x51, /* TILE_GREEN:    white on green   */
    0x61, /* TILE_BLUE:     white on blue    */
    0xB1, /* TILE_WILD:     white on d.gray  */
    0x10, /* TILE_SELECTED: black on white   */
    0x3F, /* TILE_RED_DIM:    l.gray on dark red    */
    0x4F, /* TILE_YELLOW_DIM: l.gray on dark yellow */
    0x9F, /* TILE_GREEN_DIM:  l.gray on dark green  */
    0xAF, /* TILE_BLUE_DIM:   l.gray on dark blue   */
    0xDF, /* TILE_WILD_DIM:   l.gray on dark gray   */
    0xC1  /* TILE_SEL_DIM:    white on mid gray     */
};

/* ASCII -> X16 screen code. The X16 boots in PETSCII upper/graphics mode,
   so the mapping is the classic Commodore one: A-Z -> 1-26, digits and the
   $20-$3F punctuation range map to themselves. Lowercase is folded to
   uppercase (the game only uses uppercase text). */
static unsigned char to_screen_code(unsigned char c) {
    if (c >= 'a' && c <= 'z') c = (unsigned char)(c - 32);
    if (c >= 'A' && c <= 'Z') return (unsigned char)(c - 64);
    if (c >= 0x20 && c <= 0x3F) return c;   /* space, digits, punctuation */
    return c;
}

void wait_vsync(void);   /* in vsync.s */

/* ---- sprite (card toss) ------------------------------------------------ */

/* 4-bit RGB for each suit's sprite fill, indexed COLOR_RED..COLOR_WILD. */
static const unsigned char suit_rgb[5][3] = {
    {15, 0, 0},   /* red    */
    {15, 13, 0},  /* yellow */
    {2, 13, 2},   /* green  */
    {3, 5, 15},   /* blue   */
    {8, 8, 8}     /* wild   */
};

void spr_init(void) {
    unsigned char row, col;

    /* 16x16 8bpp sprite: white 1px border, suit-colored interior. */
    vera_seek(VROM_SPRIMG, 1);
    for (row = 0; row < 16; row++) {
        for (col = 0; col < 16; col++) {
            unsigned char edge = (row == 0 || row == 15 || col == 0 || col == 15);
            VERA.data0 = edge ? SPR_EDGE : SPR_INK;
        }
    }
    set_palette(SPR_EDGE, 15, 15, 15);   /* white border */

    /* sprite 0 attributes, initially disabled (z-depth 0). */
    vera_seek(VROM_SPRATT, 1);
    VERA.data0 = (unsigned char)((VROM_SPRIMG >> 5) & 0xFF);           /* addr 12:5 */
    VERA.data0 = (unsigned char)(0x80 | ((VROM_SPRIMG >> 13) & 0x0F)); /* 8bpp | addr 16:13 */
    VERA.data0 = 0;   /* X lo */
    VERA.data0 = 0;   /* X hi */
    VERA.data0 = 0;   /* Y lo */
    VERA.data0 = 0;   /* Y hi */
    VERA.data0 = 0;   /* z-depth 0 = disabled */
    VERA.data0 = 0x50; /* height=16, width=16, palette offset 0 */

    VERA.control = 0;
    DC_VIDEO |= 0x40;  /* enable sprites layer */
}

static void spr_set_pos(unsigned char cx, unsigned char cy) {
    unsigned int px = (unsigned int)cx * 8;
    unsigned int py = (unsigned int)cy * 8;
    vera_seek(VROM_SPRATT + 2, 1);
    VERA.data0 = (unsigned char)(px & 0xFF);
    VERA.data0 = (unsigned char)(px >> 8);
    VERA.data0 = (unsigned char)(py & 0xFF);
    VERA.data0 = (unsigned char)(py >> 8);
}

void spr_show(unsigned char suit, unsigned char cx, unsigned char cy) {
    if (suit > 4) suit = 4;
    set_palette(SPR_INK, suit_rgb[suit][0], suit_rgb[suit][1], suit_rgb[suit][2]);
    spr_set_pos(cx, cy);
    vera_seek(VROM_SPRATT + 6, 1);
    VERA.data0 = 0x0C;   /* z-depth 3: in front of layer 1 */
}

void spr_move(unsigned char cx, unsigned char cy) {
    spr_set_pos(cx, cy);
}

void spr_hide(void) {
    vera_seek(VROM_SPRATT + 6, 1);
    VERA.data0 = 0x00;   /* z-depth 0: disabled */
}

/* ---- text ------------------------------------------------------------- */

void vera_init(void) {
    /* The X16 boots in the upper/lowercase text font (screen codes 1-26 =
       lowercase). Switch to the PETSCII upper/graphics font (CHR$(142)) so
       codes 1-26 render as UPPERCASE A-Z -- matching to_screen_code() and
       the iconic all-caps look of the other ports. */
    cbm_k_bsout(142);

    /* Reprogram the palette slots this driver uses for the dimmed
       "not legal to play" card tiles into darkened suit colors (VERA's
       default 0-15 are C64-ish and have no dark-suit shades). The slots
       chosen (3,4,9,10,13) are ones the UNO UI never uses for anything
       else. Also nail down the wild-tile and selection grays. */
    set_palette(3, 6, 1, 1);    /* dark red    (TILE_RED_DIM bg)    */
    set_palette(4, 6, 5, 1);    /* dark yellow (TILE_YELLOW_DIM bg) */
    set_palette(9, 1, 6, 1);    /* dark green  (TILE_GREEN_DIM bg)  */
    set_palette(10, 1, 2, 7);   /* dark blue   (TILE_BLUE_DIM bg)   */
    set_palette(11, 4, 4, 4);   /* dark gray   (TILE_WILD bg)       */
    set_palette(12, 8, 8, 8);   /* mid gray    (TILE_SEL_DIM bg)    */
    set_palette(13, 3, 3, 3);   /* darker gray (TILE_WILD_DIM bg)   */

    spr_init();
    scr_clear();
}

void scr_clear(void) {
    unsigned char y;
    unsigned char x;
    for (y = 0; y < ROWS; y++) {
        vera_seek(VROM_TEXT + (unsigned long)y * MAP_STRIDE * 2, 1);
        for (x = 0; x < COLS; x++) {
            VERA.data0 = 0x20;   /* screen code for space */
            VERA.data0 = 0x00;   /* black on black */
        }
    }
}

void scr_put(unsigned char x, unsigned char y, unsigned char ch, unsigned char color) {
    vera_seek(VROM_TEXT + ((unsigned long)y * MAP_STRIDE + x) * 2, 1);
    VERA.data0 = to_screen_code(ch);
    VERA.data0 = cell_color[color];
}

void scr_puts(unsigned char x, unsigned char y, const char *s, unsigned char color) {
    unsigned char attr = cell_color[color];
    vera_seek(VROM_TEXT + ((unsigned long)y * MAP_STRIDE + x) * 2, 1);
    while (*s) {
        VERA.data0 = to_screen_code((unsigned char)*s++);
        VERA.data0 = attr;
    }
}

void scr_put_num(unsigned char x, unsigned char y, unsigned int n, unsigned char color) {
    char buf[6];
    unsigned char i = 0;
    if (n == 0) {
        scr_put(x, y, '0', color);
        return;
    }
    while (n > 0 && i < 5) {
        buf[i++] = (char)('0' + (n % 10));
        n /= 10;
    }
    while (i > 0) {
        i--;
        scr_put(x, y, (unsigned char)buf[i], color);
        x++;
    }
}

void scr_fill_rect(unsigned char x, unsigned char y, unsigned char w, unsigned char h,
                    unsigned char ch, unsigned char color) {
    unsigned char row, col;
    unsigned char sc = to_screen_code(ch);
    unsigned char attr = cell_color[color];
    for (row = 0; row < h; row++) {
        vera_seek(VROM_TEXT + ((unsigned long)(y + row) * MAP_STRIDE + x) * 2, 1);
        for (col = 0; col < w; col++) {
            VERA.data0 = sc;
            VERA.data0 = attr;
        }
    }
}
