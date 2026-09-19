#include <conio.h>
#include <time.h>
#include "atarivid.h"

static clock_t next_tick;

void atari_init(void) {
    next_tick = clock();
    clrscr();
}

/* ANTIC does expose a CPU-visible scanline counter -- VCOUNT at $D40B --
   and the VBXE driver (vbxevid.c) polls it for a real per-frame wait.
   This build doesn't need that precision: it only paces UI animation on
   a 40x24 text screen, so it rides the OS jiffy clock (RTCLOK, reached
   through cc65's clock()) instead, the same approach as the PET port. */
void wait_vsync(void) {
    while (clock() < next_tick) {}
    next_tick = clock() + 1;
}

void scr_clear(void) {
    clrscr();
}

void scr_put(unsigned char x, unsigned char y, char ch, unsigned char inverse) {
    revers(inverse);
    cputcxy(x, y, ch);
    revers(0);
}

void scr_puts(unsigned char x, unsigned char y, const char *s, unsigned char inverse) {
    revers(inverse);
    cputsxy(x, y, s);
    revers(0);
}

void scr_put_num(unsigned char x, unsigned char y, unsigned int n) {
    char buf[6];
    unsigned char i = 0;
    if (n == 0) {
        scr_put(x, y, '0', 0);
        return;
    }
    while (n > 0 && i < 5) {
        buf[i++] = (char)('0' + (n % 10));
        n /= 10;
    }
    while (i > 0) {
        i--;
        scr_put(x, y, buf[i], 0);
        x++;
    }
}

void scr_fill_rect(unsigned char x, unsigned char y, unsigned char w, unsigned char h) {
    unsigned char row, col;
    for (row = 0; row < h; row++) {
        for (col = 0; col < w; col++) {
            scr_put((unsigned char)(x + col), (unsigned char)(y + row), ' ', 0);
        }
    }
}
