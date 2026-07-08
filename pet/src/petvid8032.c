#include <string.h>
#include <time.h>
#include <ascii_charmap.h>
#include "petvid8032.h"

static clock_t next_tick;

void pet_init(void) {
    next_tick = clock();
    scr_clear();
}

/* The PET has no CPU-visible vsync/raster signal to poll (unlike the VIC-II
   or TED), so instead of wait_vsync() we pace at a fixed rate using the
   KERNAL jiffy clock via cc65's clock(). */
void wait_tick(void) {
    while (clock() < next_tick) {}
    next_tick = clock() + 1;
}

static unsigned char ascii_to_screencode(char c) {
    unsigned char u = (unsigned char)c;
    if (u >= 32 && u <= 63) return u;
    if (u >= 64 && u <= 95) return u - 64;
    if (u >= 97 && u <= 122) return u - 96;
    return 32;
}

void scr_clear(void) {
    memset(SCREEN, 32, (unsigned int)COLS * ROWS);
}

void scr_put(unsigned char x, unsigned char y, unsigned char ch) {
    unsigned int off = (unsigned int)y * COLS + x;
    SCREEN[off] = ascii_to_screencode((char)ch);
}

void scr_puts(unsigned char x, unsigned char y, const char *s) {
    unsigned int off = (unsigned int)y * COLS + x;
    while (*s) {
        SCREEN[off] = ascii_to_screencode(*s);
        off++;
        s++;
    }
}

void scr_put_num(unsigned char x, unsigned char y, unsigned int n) {
    char buf[6];
    unsigned char i = 0;
    if (n == 0) {
        scr_put(x, y, '0');
        return;
    }
    while (n > 0 && i < 5) {
        buf[i++] = (char)('0' + (n % 10));
        n /= 10;
    }
    while (i > 0) {
        i--;
        scr_put(x, y, buf[i]);
        x++;
    }
}

void scr_fill_rect(unsigned char x, unsigned char y, unsigned char w, unsigned char h,
                    unsigned char ch) {
    unsigned char row, col;
    for (row = 0; row < h; row++) {
        for (col = 0; col < w; col++) {
            scr_put(x + col, y + row, ch);
        }
    }
}
