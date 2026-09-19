#include <string.h>
#include "ted.h"

#define TED_BORDER (*(unsigned char *)0xFF19)
#define TED_BG (*(unsigned char *)0xFF15)
#define TED_RASTER_LO (*(unsigned char *)0xFF1D)

/* $FF1D holds only the LOW 8 BITS of TED's 9-bit raster counter
   (0-311 PAL, 0-261 NTSC), so `== 0` matches line 0 AND line 256: the old
   `!= 0` / `== 0` pair completed TWICE per frame, at uneven 256-line and
   56-line intervals, which paced every animation and sound duration at
   roughly double speed and with visible judder (measured in VICE: 100
   waits took 60 jiffies where a true frame wait took 121).
   Waiting on a high band instead needs only the low byte: 240-255 occurs
   exactly once per frame on both standards, because 496-511 never happens.
   Same idiom as mega65/src/mega65vid.c, which had it right all along. */
#define RASTER_BAND 0xF0
void wait_vsync(void) {
    while (TED_RASTER_LO >= RASTER_BAND) {}
    while (TED_RASTER_LO < RASTER_BAND) {}
}

static unsigned char ascii_to_screencode(char c) {
    unsigned char u = (unsigned char)c;
    if (u >= 32 && u <= 63) return u;
    if (u >= 64 && u <= 95) return u - 64;
    if (u >= 97 && u <= 122) return u - 96;
    return 32;
}

void ted_init(void) {
    TED_BORDER = COL_BLACK;
    TED_BG = COL_BLACK;
    scr_clear();
}

void scr_clear(void) {
    memset(SCREEN, 32, COLS * ROWS);
    memset(COLOR, COL_WHITE, COLS * ROWS);
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
