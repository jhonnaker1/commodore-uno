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

#define VIC_REG2 (*(unsigned char *)0x9002)
#define VIC_REG5 (*(unsigned char *)0x9005)
#define VIC_REG15 (*(unsigned char *)0x900F)

void vic20_init(void) {
    /* Redirect the VIC's video matrix back to $1E00 (see the comment on
       SCREEN in vic20io.h) instead of wherever the KERNAL relocated it to
       when it detected expansion RAM. Values taken directly from the
       working unexpanded config's own registers, not computed from a
       formula (an earlier attempt to compute an alternate address that
       way produced garbage). */
    VIC_REG2 = (VIC_REG2 & 0x7F) | 0x80;
    VIC_REG5 = (VIC_REG5 & 0x0F) | 0xF0;

    /* The KERNAL's actual unexpanded default is a WHITE background/cyan
       border (confirmed via the VICE monitor: $900F reads $1B), not the
       "classic VIC-20 blue" an earlier comment here assumed -- that
       assumption was never verified and left the whole title screen
       rendering as invisible white-on-white text. Force a black
       background/border instead, matching every other platform in this
       project (dark background, light text) so the existing color
       choices in ui.c actually show up. Bit 3 must stay set (screen
       normal, not reverse); bits 0-2 = border color, bits 4-7 = background
       color, both 0 = black. */
    VIC_REG15 = 0x08;

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
