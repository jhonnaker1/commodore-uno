#ifndef TED_H
#define TED_H

/* We deliberately use the Plus/4's default screen matrix and the built-in
   ROM character set (no relocation, no custom card-art glyphs) -- TED's
   character-generator ROM/RAM select touches the same register range as
   ROM banking, and getting that wrong hangs the machine. Cards are shown
   as plain colored text instead of drawn boxes. */
#define SCREEN ((unsigned char *)0x0C00)
#define COLOR ((unsigned char *)0x0800)
#define COLS 40
#define ROWS 25

/* TED color bytes are (hue | luminance); hardcoded here rather than
   pulling in cbm264.h's own COLOR_* macros, which collide by name (not
   value) with cards.h's COLOR_RED/YELLOW/GREEN/BLUE suit constants. */
#define COL_BLACK 0x00
#define COL_WHITE 0x71
#define COL_RED 0x42
#define COL_CYAN 0x73
#define COL_PURPLE 0x74
#define COL_GREEN 0x75
#define COL_BLUE 0x76
#define COL_YELLOW 0x77
#define COL_ORANGE 0x78
#define COL_LTGRAY 0x51
#define COL_MDGRAY 0x31
#define COL_DKGRAY 0x11

void ted_init(void);
void wait_vsync(void);
void scr_clear(void);
void scr_put(unsigned char x, unsigned char y, unsigned char ch, unsigned char color);
void scr_puts(unsigned char x, unsigned char y, const char *s, unsigned char color);
void scr_put_num(unsigned char x, unsigned char y, unsigned int n, unsigned char color);
void scr_fill_rect(unsigned char x, unsigned char y, unsigned char w, unsigned char h,
                    unsigned char ch, unsigned char color);

#endif
