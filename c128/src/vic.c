#include <string.h>
#include "vic.h"
#include "charset_data.h"

#define CIA2_PRA (*(unsigned char *)0xDD00)
#define VIC_MEMCTL (*(unsigned char *)0xD018)
#define VIC_BORDER (*(unsigned char *)0xD020)
#define VIC_BG0 (*(unsigned char *)0xD021)
#define VIC_MCM (*(unsigned char *)0xD016)
#define VIC_RASTER (*(unsigned char *)0xD012)

/* KERNAL zero-page screen-editor variables (same layout as C64): PNT
   (current screen line address, lo/hi) and PNTR (cursor column). */
#define KERNAL_PNT_LO (*(unsigned char *)0xD1)
#define KERNAL_PNT_HI (*(unsigned char *)0xD2)
#define KERNAL_PNTR (*(unsigned char *)0xD3)

/* The KERNAL re-asserts the *entire* $D018 byte back to its own default
   ($17) periodically -- confirmed by writing a different char-base value
   and reading it straight back as $17 again. We need to disagree with it
   on the char-base bits (see vic_init()), so we have to keep re-winning
   that fight every frame rather than just building to its preferred
   value the way we do for the screen-address bits. */
void wait_vsync(void) {
    while (VIC_RASTER != 0) {}
    while (VIC_RASTER == 0) {}
    VIC_MEMCTL = 0x18;
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

    /* The C128 KERNAL's own IRQ periodically re-asserts $D018's screen-
       address bits back to its stock default (screen at bank offset
       $400), so we build to that instead of fighting it every frame --
       see SCREEN in vic.h. The char-base bits are ours to choose freely
       though, and offset $1800 (the KERNAL's own default there) is a
       trap: on VIC bank 0 or 2, char-base offsets $1000 and $1800 are
       hardwired to show the internal character ROM, ignoring RAM
       entirely -- which is why custom glyphs 128-255 were silently
       aliasing to glyphs 0-127 (that ROM range's upper half is just a
       reverse-video mirror of its lower half). Offset $2000 avoids it. */
    VIC_MEMCTL = 0x18;

    /* Plain hires text mode (no multicolor, no extended color). */
    VIC_MCM &= (unsigned char)~0x10;

    memcpy((void *)0xA000, charset_data, 2048);

    VIC_BORDER = COL_BLACK;
    VIC_BG0 = COL_BLACK;

    /* The KERNAL's screen editor still thinks the screen is at its
       default $0400 (it was never told otherwise), and its IRQ-driven
       cursor housekeeping apparently depends on that being accurate --
       plausibly why the whole keyboard buffer goes dead once the screen
       actually lives at $8400 instead. Point it at the real location. */
    KERNAL_PNT_LO = (unsigned char)((unsigned int)SCREEN & 0xFF);
    KERNAL_PNT_HI = (unsigned char)((unsigned int)SCREEN >> 8);
    KERNAL_PNTR = 0;

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
