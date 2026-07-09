#include <conio.h>
#include <time.h>
#include "vid510.h"

static clock_t next_tick;

void cbm510_init(void) {
    next_tick = clock();
    /* Default background is white (confirmed empirically -- COL_WHITE
       text was invisible until this was added), unlike every other
       Commodore port's black default, so force it explicitly rather than
       hunting down every COL_WHITE usage in ui.c. */
    bgcolor(COL_BLACK);
    bordercolor(COL_BLACK);
    clrscr();
}

/* No CPU-visible vsync/raster signal reachable without the same bank-
   switching complexity the video/color RAM needed (see vid510.h), so
   pace with the KERNAL jiffy clock instead, same approach as the PET. */
void wait_vsync(void) {
    while (clock() < next_tick) {}
    next_tick = clock() + 1;
}

void scr_clear(void) {
    clrscr();
}

void scr_put(unsigned char x, unsigned char y, unsigned char ch, unsigned char color) {
    textcolor(color);
    cputcxy(x, y, ch);
}

void scr_puts(unsigned char x, unsigned char y, const char *s, unsigned char color) {
    textcolor(color);
    cputsxy(x, y, s);
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
        scr_put(x, y, buf[i], color);
        x++;
    }
}

void scr_fill_rect(unsigned char x, unsigned char y, unsigned char w, unsigned char h,
                    unsigned char ch, unsigned char color) {
    unsigned char row, col;
    textcolor(color);
    for (row = 0; row < h; row++) {
        for (col = 0; col < w; col++) {
            cputcxy((unsigned char)(x + col), (unsigned char)(y + row), ch);
        }
    }
}

void scr_put_solid(unsigned char x, unsigned char y, unsigned char color) {
    textcolor(color);
    revers(1);
    cputcxy(x, y, ' ');
    revers(0);
}
