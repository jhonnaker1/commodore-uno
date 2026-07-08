#ifndef VIC20IO_H
#define VIC20IO_H

/* Confirmed empirically (via VICE's monitor) for the 32K-expansion memory
   config: the KERNAL relocates the screen matrix to $1000 by default, but
   the real VIC-II chip has a genuine, reproducible rendering bug at that
   address (large parts of a correct, in-memory screen just don't display,
   independent of RAM amount or KERNAL). $1E00 -- the stock/unexpanded
   default -- renders correctly, so vic20_init() redirects the VIC's
   video-matrix registers back there instead of using the KERNAL's
   relocated default. The linker config (vic20-highmem.cfg) deliberately
   leaves $1000-$1FFF free of code/data so this is safe to write to. Color
   RAM is always fixed at $9600 regardless of expansion. We use the stock
   KERNAL character set (no relocation) -- same reasoning as the Plus/4 port. */
#define SCREEN ((unsigned char *)0x1E00)
#define COLOR ((unsigned char *)0x9600)
#define COLS 22
#define ROWS 23

/* Plain 0-15 palette, matching vic20.h's own COLOR_* numerically but
   under our own names to avoid clashing with cards.h's COLOR_RED etc. */
/* Colors 0-7 render as plain single-color text. Colors 8-15 have bit 3
   set, which switches that character cell into VIC multicolor mode (a
   totally different 4-color/2-bit-per-pixel rendering using shared
   background/aux registers) instead of just being "a brighter shade" --
   using them for ordinary text corrupts the glyph. So every color we use
   here is deliberately kept in 0-7; the "gray"/"orange" names below are
   just aliased to safe, visually-distinct low colors instead. */
#define COL_BLACK 0x00
#define COL_WHITE 0x01
#define COL_RED 0x02
#define COL_CYAN 0x03
#define COL_PURPLE 0x04
#define COL_GREEN 0x05
#define COL_BLUE 0x06
#define COL_YELLOW 0x07
#define COL_ORANGE 0x04
#define COL_LTGRAY 0x03
#define COL_MDGRAY 0x03
#define COL_DKGRAY 0x04

void vic20_init(void);
void wait_vsync(void);
void scr_clear(void);
void scr_put(unsigned char x, unsigned char y, unsigned char ch, unsigned char color);
void scr_puts(unsigned char x, unsigned char y, const char *s, unsigned char color);
void scr_put_num(unsigned char x, unsigned char y, unsigned int n, unsigned char color);
void scr_fill_rect(unsigned char x, unsigned char y, unsigned char w, unsigned char h,
                    unsigned char ch, unsigned char color);

#endif
