#include <mega65/conio.h>
#include "m65native.h"

#define REV 0x20   /* ATTRIB_REVERSE */

/* PETSCII -> C64/MEGA65 screen code. cputcxy() writes raw screen codes, so
   letters ($41-$5A), the box-drawing graphics ($A0-$FF) and control-range
   codes all need translating; digits/space/punctuation already match.

   Identical to the 40-column build's converter, and for a slightly
   different reason. There, cc65 would have applied its own PETSCII charmap
   to string literals, so ui.c had to include <ascii_charmap.h> to keep them
   ASCII for this function to work on. llvm-mos has no charmap concept at
   all -- literals are plain ASCII already -- so the native UI needs no such
   include, and the same conversion table is correct for both. */
static unsigned char sc(unsigned char c) {
    if (c <= 0x1F) return (unsigned char)(c + 0x80);
    if (c <= 0x3F) return c;
    if (c <= 0x5F) return (unsigned char)(c - 0x40);
    if (c <= 0x7F) return (unsigned char)(c - 0x20);
    if (c <= 0x9F) return (unsigned char)(c + 0x40);
    if (c <= 0xBF) return (unsigned char)(c - 0x40);
    return (unsigned char)(c - 0x80);
}

void mega65_init(void) {
    conioinit();
    /* The line that the C64-mode build cannot make stick. In native mode
       this sets VIC-IV's H640 bit for real, and mega65-libc also applies
       the VIC-III H640 horizontal-positioning fix ($D04C) along with it. */
    setscreensize(COLS, ROWS);
    setextendedattrib(1);   /* enable reverse attribute for colour swatches */
    setuppercase();
    bordercolor(COL_BLACK);
    bgcolor(COL_BLACK);
    clrscr();
}

/* Frame pace off the VIC raster. $D012 is the VIC-II-compatible low byte of
   the raster counter, which the VIC-IV still maintains in native mode. */
void wait_vsync(void) {
    while (*(volatile unsigned char *)0xD012 >= 0xF0) { }
    while (*(volatile unsigned char *)0xD012 < 0xF0) { }
}

void scr_clear(void) {
    clrscr();
}

void scr_put(unsigned char x, unsigned char y, unsigned char ch, unsigned char color) {
    cputcxy(x, y, sc(ch));
    cellcolor(x, y, color);
}

void scr_puts(unsigned char x, unsigned char y, const char *s, unsigned char color) {
    while (*s) {
        cputcxy(x, y, sc((unsigned char)*s++));
        cellcolor(x, y, color);
        x++;
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
    unsigned char screen_ch = sc(ch);
    for (row = 0; row < h; row++)
        for (col = 0; col < w; col++) {
            cputcxy((unsigned char)(x + col), (unsigned char)(y + row), screen_ch);
            cellcolor((unsigned char)(x + col), (unsigned char)(y + row), color);
        }
}

void scr_put_solid(unsigned char x, unsigned char y, unsigned char color) {
    cputcxy(x, y, sc(' '));
    cellcolor(x, y, (unsigned char)(color | REV));
}
