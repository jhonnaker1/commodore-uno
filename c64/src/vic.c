#include <string.h>
#include "vic.h"
#include "charset_data.h"

#define CIA2_PRA (*(unsigned char *)0xDD00)
#define VIC_MEMCTL (*(unsigned char *)0xD018)
#define VIC_BORDER (*(unsigned char *)0xD020)
#define VIC_BG0 (*(unsigned char *)0xD021)
#define VIC_MCM (*(unsigned char *)0xD016)
#define VIC_RASTER (*(unsigned char *)0xD012)

#define VIC_SPR0_X (*(unsigned char *)0xD000)
#define VIC_SPR0_Y (*(unsigned char *)0xD001)
#define VIC_SPR_MSB (*(unsigned char *)0xD010)
#define VIC_SPR_ENABLE (*(unsigned char *)0xD015)
#define VIC_SPR0_COLOR (*(unsigned char *)0xD027)

#define SPRITE_DATA_ADDR 0x9000

/* A simple filled card silhouette, 24x21, corners trimmed on the first/last
   rows. Color is set per-use to match whatever card is being tossed. */
static const unsigned char sprite_data[63] = {
    0x00, 0x00, 0x00,
    0x0F, 0xFF, 0xF0,
    0x3F, 0xFF, 0xFC, 0x3F, 0xFF, 0xFC, 0x3F, 0xFF, 0xFC, 0x3F, 0xFF, 0xFC,
    0x3F, 0xFF, 0xFC, 0x3F, 0xFF, 0xFC, 0x3F, 0xFF, 0xFC, 0x3F, 0xFF, 0xFC,
    0x3F, 0xFF, 0xFC, 0x3F, 0xFF, 0xFC, 0x3F, 0xFF, 0xFC, 0x3F, 0xFF, 0xFC,
    0x3F, 0xFF, 0xFC, 0x3F, 0xFF, 0xFC, 0x3F, 0xFF, 0xFC,
    0x0F, 0xFF, 0xF0,
    0x00, 0x00, 0x00
};

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

    /* Screen matrix at bank offset 0 ($8000), char base at offset $800 ($8800). */
    VIC_MEMCTL = 0x02;

    /* Plain hires text mode (no multicolor, no extended color). */
    VIC_MCM &= (unsigned char)~0x10;

    memcpy((void *)0x8800, charset_data, 2048);

    VIC_BORDER = COL_BLACK;
    VIC_BG0 = COL_BLACK;

    memcpy((void *)SPRITE_DATA_ADDR, sprite_data, 63);
    /* Sprite pointer bytes live at the end of the 1K screen block VIC-II
       expects (screen base + $3F8..$3FF), even though we only use 1000 of
       those 1024 bytes for the visible 40x25 matrix. */
    SCREEN[0x3F8] = (unsigned char)((SPRITE_DATA_ADDR - 0x8000) / 64);
    VIC_SPR_ENABLE = 0;

    scr_clear();
}

void sprite_enable(unsigned char on) {
    if (on) {
        VIC_SPR_ENABLE |= 0x01;
    } else {
        VIC_SPR_ENABLE &= (unsigned char)~0x01;
    }
}

void sprite_set_color(unsigned char color) {
    VIC_SPR0_COLOR = color;
}

void sprite_set_pos(unsigned int x, unsigned char y) {
    VIC_SPR0_X = (unsigned char)(x & 0xFF);
    if (x > 255) {
        VIC_SPR_MSB |= 0x01;
    } else {
        VIC_SPR_MSB &= (unsigned char)~0x01;
    }
    VIC_SPR0_Y = y;
}

unsigned int sprite_x_from_col(unsigned char col) {
    return 24 + (unsigned int)col * 8;
}

unsigned char sprite_y_from_row(unsigned char row) {
    return (unsigned char)(50 + (unsigned int)row * 8);
}

void vic_set_border(unsigned char color) {
    VIC_BORDER = color;
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
