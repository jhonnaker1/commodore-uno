#include <mega65/conio.h>
#include "mega65vid.h"

#define REV 0x20   /* ATTRIB_REVERSE */

/* PETSCII -> C64/MEGA65 screen code. cputcxy() writes raw screen codes, so
   letters ($41-$5A), the box-drawing graphics ($A0-$FF) and control-range
   codes all need translating; digits/space/punctuation already match. */
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
    setextendedattrib(1);   /* enable reverse attribute for colour swatches */
    setuppercase();
    bordercolor(COL_BLACK);
    bgcolor(COL_BLACK);
    clrscr();
}

/* Frame pace off the VIC raster (C64-compatible $D012 low byte). */
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
