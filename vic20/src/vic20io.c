#include <string.h>
#include <vic20.h>
#include "vic20io.h"

void wait_vsync(void) {
    while (VIC.rasterline != 0) {}
    while (VIC.rasterline == 0) {}
}

static unsigned char ascii_to_screencode(char c) {
    unsigned char u = (unsigned char)c;
    if (u >= 32 && u <= 63) return u;
    if (u >= 64 && u <= 95) return u - 64;
    if (u >= 97 && u <= 122) return u - 96;
    return 32;
}

void vic20_init(void) {
    /* Deliberately leaving the KERNAL's default background/border color
       alone (the classic VIC-20 blue) rather than fighting whatever makes
       that register behave unexpectedly -- text colors are chosen to read
       well against it instead. */
    scr_clear();
}

void scr_clear(void) {
    memset(SCREEN, 32, (unsigned int)COLS * ROWS);
    memset(COLOR, COL_WHITE, (unsigned int)COLS * ROWS);
}

void scr_put(unsigned char x, unsigned char y, unsigned char ch, unsigned char color) {
    unsigned int off = (unsigned int)y * COLS + x;
    if (ch < 128) ch = ascii_to_screencode(ch);
    SCREEN[off] = ch;
    COLOR[off] = color;
}

void scr_puts(unsigned char x, unsigned char y, const char *s, unsigned char color) {
    unsigned int off = (unsigned int)y * COLS + x;
    while (*s) {
        SCREEN[off] = ascii_to_screencode(*s);
        COLOR[off] = color;
        off++;
        s++;
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
        scr_put(x, y, buf[i], color);
        x++;
    }
}

void scr_fill_rect(unsigned char x, unsigned char y, unsigned char w, unsigned char h,
                    unsigned char ch, unsigned char color) {
    unsigned char row, col;
    for (row = 0; row < h; row++) {
        for (col = 0; col < w; col++) {
            scr_put(x + col, y + row, ch, color);
        }
    }
}
