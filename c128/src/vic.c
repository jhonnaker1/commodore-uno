#include <string.h>
#include "vic.h"
#include "charset_data.h"

#define CIA2_PRA (*(unsigned char *)0xDD00)
#define VIC_MEMCTL (*(unsigned char *)0xD018)
#define VIC_BORDER (*(unsigned char *)0xD020)
#define VIC_BG0 (*(unsigned char *)0xD021)
#define VIC_MCM (*(unsigned char *)0xD016)
#define VIC_RASTER (*(unsigned char *)0xD012)

void wait_vsync(void) {
    while (VIC_RASTER != 0) {}
    while (VIC_RASTER == 0) {}
}

static unsigned char ascii_to_screencode(char c) {
    unsigned char u = (unsigned char)c;
    if (u >= 32 && u <= 63) return u;
    if (u >= 64 && u <= 95) return u - 64;
    if (u >= 97 && u <= 122) return u - 96; /* lowercase -> same glyphs as upper */
    return 32;
}

void vic_init(void) {
    /* Select VIC bank 2 ($8000-$BFFF): bits 0-1 of CIA2 PRA, value %01. */
    CIA2_PRA = (CIA2_PRA & 0xFC) | 0x01;

    /* The C128 KERNAL's own IRQ periodically re-asserts $D018 back to its
       stock default ($17 -- screen at bank offset $400, char base at
       offset $1800) regardless of what we write here; it's never told
       our screen/charset live elsewhere, so it keeps fighting any other
       value. Rather than fight it every frame, we just build to the
       addresses it already insists on: screen at $8400, char base at
       $9800 (see SCREEN in vic.h). */
    VIC_MEMCTL = 0x17;

    /* Plain hires text mode (no multicolor, no extended color). */
    VIC_MCM &= (unsigned char)~0x10;

    memcpy((void *)0x9800, charset_data, 2048);

    VIC_BORDER = COL_BLACK;
    VIC_BG0 = COL_BLACK;

    scr_clear();
}

void scr_clear(void) {
    memset(SCREEN, 32, 1000);
    memset(COLOR, COL_WHITE, 1000);
}

void scr_put(unsigned char x, unsigned char y, unsigned char ch, unsigned char color) {
    unsigned int off = (unsigned int)y * 40 + x;
    /* Values 0-127 are treated as ASCII (e.g. char literals like '0'+n);
       128-255 are our custom card-art glyph codes and pass through as-is. */
    if (ch < 128) ch = ascii_to_screencode(ch);
    SCREEN[off] = ch;
    COLOR[off] = color;
}

void scr_puts(unsigned char x, unsigned char y, const char *s, unsigned char color) {
    unsigned int off = (unsigned int)y * 40 + x;
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
