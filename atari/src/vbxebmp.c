#include <string.h>
#include "vbxebmp.h"

/* ---- VBXE registers ($D6xx base), same as the text driver ---- */
#define VBXE_BASE 0xD640
#define VIDEO_CONTROL (*(unsigned char *)(VBXE_BASE + 0x00))
#define XDL_ADR0 (*(unsigned char *)(VBXE_BASE + 0x01))
#define XDL_ADR1 (*(unsigned char *)(VBXE_BASE + 0x02))
#define XDL_ADR2 (*(unsigned char *)(VBXE_BASE + 0x03))
#define CSEL (*(unsigned char *)(VBXE_BASE + 0x04))
#define PSEL (*(unsigned char *)(VBXE_BASE + 0x05))
#define CR (*(unsigned char *)(VBXE_BASE + 0x06))
#define CG (*(unsigned char *)(VBXE_BASE + 0x07))
#define CB (*(unsigned char *)(VBXE_BASE + 0x08))
#define MEMAC_CONTROL (*(unsigned char *)(VBXE_BASE + 0x1E))
#define MEMAC_BANK_SEL (*(unsigned char *)(VBXE_BASE + 0x1F))
#define VIC_RASTER (*(unsigned char *)0xD40B)   /* ANTIC VCOUNT, still ticks under VBXE */

#define MEMAC_A ((unsigned char *)0x2000)

/* VRAM layout: XDL at $0000, framebuffer at $1000. The 320*192 = 61440-byte
   framebuffer runs $1000..$FFFF -- i.e. it fits entirely within the low 64K
   of VBXE VRAM, so pixel addressing uses fast 16-bit `unsigned int` math and
   only the MEMAC bank register selects which 8K slice the window shows. */
#define VRAM_XDL 0x00000UL
#define VRAM_FB 0x1000            /* 16-bit VRAM address of the framebuffer */

static unsigned char current_bank = 0xFF;

/* 32-bit bank select, used only by init() for the XDL near VRAM $0000. */
static void select_bank(unsigned long vram_addr) {
    unsigned char bank = (unsigned char)((vram_addr >> 12) & 0x7E);
    if (bank != current_bank) {
        MEMAC_CONTROL = 0x29;   /* window at $2000, CPU enable, 8K */
        MEMAC_BANK_SEL = (unsigned char)(0x80 | bank);
        current_bank = bank;
    }
}

/* Fast 16-bit bank select for framebuffer addresses ($1000..$FFFF). */
static void select_bank16(unsigned int addr) {
    unsigned char bank = (unsigned char)((addr >> 12) & 0x7E);
    if (bank != current_bank) {
        MEMAC_CONTROL = 0x29;
        MEMAC_BANK_SEL = (unsigned char)(0x80 | bank);
        current_bank = bank;
    }
}

/* Fill `count` framebuffer bytes from 16-bit VRAM `start`, crossing 8K banks
   via memset. `start`+`count` never exceeds $10000 for valid framebuffer
   spans (the last row ends exactly at the window's top), so the unsigned
   wrap to 0 harmlessly terminates the loop. */
static void fb_fill16(unsigned int start, unsigned int count, unsigned char color) {
    while (count) {
        unsigned int off, chunk;
        select_bank16(start);
        off = start & 0x1FFF;
        chunk = 0x2000 - off;
        if (chunk > count) chunk = count;
        memset(MEMAC_A + off, color, chunk);
        start += chunk;
        count -= chunk;
    }
}

static void vram_write(unsigned long vram_addr, const unsigned char *data, unsigned int len) {
    unsigned int off;
    for (off = 0; off < len; off++) {
        unsigned long a = vram_addr + off;
        select_bank(a);
        MEMAC_A[a & 0x1FFF] = data[off];
    }
}

void vbmp_palette(unsigned char idx, unsigned char r, unsigned char g, unsigned char b) {
    PSEL = 1;
    CSEL = idx;
    CR = r;
    CG = g;
    CB = b;
}

void vbmp_wait_vsync(void) {
    unsigned char start = VIC_RASTER;
    while (VIC_RASTER == start) { }
}

void vbmp_init(void) {
    /* SR (320x192, 8bpp) overlay XDL. Structure mirrors the working text
       XDL (vbxevid.c) but the graphics entry uses GMON (xdl1 & 3 == 2,
       bit1) instead of text's TMON, drops CHBASE, and OVSTEP is the pixel
       row stride (320). Entry 1 is an 8-scanline overlay-off border that
       programs OVADR + OVATT; entry 2 turns the bitmap overlay on for the
       192 visible scanlines (OVADR auto-steps by OVSTEP each scanline in
       bitmap mode). */
    unsigned char entry[11];
    unsigned char n;
    unsigned int step = BMP_W;             /* 320 bytes per row */
    unsigned long xdl_off = VRAM_XDL;
    unsigned long ovadr = VRAM_FB;

    /* Entry 1: overlay off (xdl1 bits0-1 = 0), set OVADR + OVATT.
       xdl1: MAPOFF 0x10 | RPTL 0x20 | OVADR 0x40 = 0x70. xdl2: OVATT 0x08. */
    n = 0;
    entry[n++] = 0x70;
    entry[n++] = 0x08;
    entry[n++] = 7;                                    /* RPTL: 8 scanlines */
    entry[n++] = (unsigned char)(ovadr & 0xFF);        /* OVADR */
    entry[n++] = (unsigned char)((ovadr >> 8) & 0xFF);
    entry[n++] = (unsigned char)((ovadr >> 16) & 0x07);
    entry[n++] = (unsigned char)(step & 0xFF);         /* OVSTEP (12-bit) */
    entry[n++] = (unsigned char)((step >> 8) & 0x0F);
    entry[n++] = 0x11;                                 /* OVATT: NORMAL width + palette 1 */
    entry[n++] = 255;                                  /* OVATT priority */
    vram_write(xdl_off, entry, n);
    xdl_off += n;

    /* Entry 2: GMON bitmap on for BMP_H scanlines, END.
       xdl1: GMON 0x02 | MAPOFF 0x10 | RPTL 0x20 | OVADR 0x40 = 0x72
       (bit2 clear -> SR not HR). xdl2: END 0x80 (mode column 0 -> SR). */
    n = 0;
    entry[n++] = 0x72;
    entry[n++] = 0x80;
    entry[n++] = (unsigned char)(BMP_H - 1);           /* RPTL: 192 scanlines */
    entry[n++] = (unsigned char)(ovadr & 0xFF);
    entry[n++] = (unsigned char)((ovadr >> 8) & 0xFF);
    entry[n++] = (unsigned char)((ovadr >> 16) & 0x07);
    entry[n++] = (unsigned char)(step & 0xFF);
    entry[n++] = (unsigned char)((step >> 8) & 0x0F);
    vram_write(xdl_off, entry, n);

    XDL_ADR0 = (unsigned char)(VRAM_XDL & 0xFF);
    XDL_ADR1 = (unsigned char)((VRAM_XDL >> 8) & 0xFF);
    XDL_ADR2 = (unsigned char)((VRAM_XDL >> 16) & 0x07);

    VIDEO_CONTROL = 0x01 | 0x04;     /* xdl_enabled + no_trans */
    *(unsigned char *)0x022F = 0;    /* SDMCTL: disable ANTIC playfield DMA */
}

void vbmp_clear(unsigned char color) {
    /* Full screen: iterate rows so each fb_fill16 span stays within 16 bits. */
    unsigned int base = VRAM_FB;
    unsigned char y;
    for (y = 0; y < BMP_H; y++) {
        fb_fill16(base, BMP_W, color);
        base += BMP_W;
    }
}

void vbmp_pixel(unsigned int x, unsigned char y, unsigned char color) {
    unsigned int a = VRAM_FB + (unsigned int)y * BMP_W + x;
    select_bank16(a);
    MEMAC_A[a & 0x1FFF] = color;
}

void vbmp_hline(unsigned int x, unsigned char y, unsigned int w, unsigned char color) {
    fb_fill16(VRAM_FB + (unsigned int)y * BMP_W + x, w, color);
}

void vbmp_fill_rect(unsigned int x, unsigned char y, unsigned int w, unsigned char h, unsigned char color) {
    unsigned int base = VRAM_FB + (unsigned int)y * BMP_W + x;
    unsigned char row;
    for (row = 0; row < h; row++) {
        fb_fill16(base, w, color);
        base += BMP_W;
    }
}

void vbmp_frame_rect(unsigned int x, unsigned char y, unsigned int w, unsigned char h, unsigned char color) {
    unsigned char row;
    vbmp_hline(x, y, w, color);
    vbmp_hline(x, (unsigned char)(y + h - 1), w, color);
    for (row = 1; row < h - 1; row++) {
        vbmp_pixel(x, (unsigned char)(y + row), color);
        vbmp_pixel((unsigned int)(x + w - 1), (unsigned char)(y + row), color);
    }
}

/* ---- font (ROM charset blitted into the framebuffer) ---- */

/* ASCII -> Atari internal screen code (the ROM charset is in screen-code
   order). Same mapping the text driver uses. */
static unsigned char to_screen_code(unsigned char c) {
    if (c < 0x20) return (unsigned char)(c + 0x40);
    if (c < 0x60) return (unsigned char)(c - 0x20);
    return c;
}

void vbmp_char(unsigned int x, unsigned char y, unsigned char ch, unsigned char fg) {
    /* ROM charset at $E000 (CHBAS default), 8 bytes per glyph, MSB = left. */
    const unsigned char *glyph = (const unsigned char *)0xE000 + (unsigned int)to_screen_code(ch) * 8;
    unsigned int base = VRAM_FB + (unsigned int)y * BMP_W + x;
    unsigned char row, b, col;
    for (row = 0; row < 8; row++) {
        b = glyph[row];
        if (b) {
            for (col = 0; col < 8; col++) {
                if (b & (0x80 >> col)) {
                    unsigned int a = base + col;
                    select_bank16(a);
                    MEMAC_A[a & 0x1FFF] = fg;
                }
            }
        }
        base += BMP_W;
    }
}

void vbmp_text(unsigned int x, unsigned char y, const char *s, unsigned char fg) {
    while (*s) {
        vbmp_char(x, y, (unsigned char)*s++, fg);
        x += 8;
    }
}

/* ---- card art ---- */

/* card action-value codes (from cards.h; not included here to avoid the
   COLOR_* clash pattern -- this file only needs the value codes) */
#define VAL_SKIP 10
#define VAL_REVERSE 11
#define VAL_DRAW2 12
#define VAL_WILD 13
#define VAL_WILD4 14

/* suit (0-4) -> palette index */
static const unsigned char suit_col[5] = {VC_RED, VC_YELLOW, VC_GREEN, VC_BLUE, VC_GRAY};

#define CARD_W VBMP_CARD_W
#define CARD_H VBMP_CARD_H

static char value_char(unsigned char v) {
    if (v <= 9) return (char)('0' + v);
    switch (v) {
        case VAL_SKIP: return 'S';
        case VAL_REVERSE: return 'R';
        case VAL_DRAW2: return 'D';
        case VAL_WILD: return 'W';
        default: return 'F';
    }
}

void vbmp_card(unsigned int x, unsigned char y, unsigned char suit, unsigned char value) {
    unsigned char sc = suit_col[suit <= 4 ? suit : 4];
    char vc = value_char(value);

    /* Border is fill-based (fast) rather than per-pixel outlines: a full
       suit-coloured rect, then a white body inset by 3px leaves a border. */
    vbmp_fill_rect(x + 3, y + 3, CARD_W, CARD_H, VC_SHADOW);      /* drop shadow */
    vbmp_fill_rect(x, y, CARD_W, CARD_H, sc);                     /* suit border */
    vbmp_fill_rect(x + 3, y + 3, CARD_W - 6, CARD_H - 6, VC_WHITE); /* body */
    vbmp_char(x + 4, y + 4, (unsigned char)vc, sc);              /* top-left value */
    vbmp_char(x + CARD_W - 12, y + CARD_H - 12, (unsigned char)vc, sc); /* bottom-right */
    /* centre badge: suit block with the value in white */
    vbmp_fill_rect(x + CARD_W / 2 - 8, y + CARD_H / 2 - 9, 16, 18, sc);
    vbmp_char(x + CARD_W / 2 - 4, y + CARD_H / 2 - 4, (unsigned char)vc, VC_WHITE);
}

void vbmp_card_back(unsigned int x, unsigned char y) {
    vbmp_fill_rect(x + 3, y + 3, CARD_W, CARD_H, VC_SHADOW);
    vbmp_fill_rect(x, y, CARD_W, CARD_H, VC_WHITE);
    vbmp_fill_rect(x + 3, y + 3, CARD_W - 6, CARD_H - 6, VC_RED);   /* red field */
    vbmp_fill_rect(x + CARD_W / 2 - 7, y + CARD_H / 2 - 6, 14, 12, VC_WHITE); /* logo box */
    vbmp_char(x + CARD_W / 2 - 4, y + CARD_H / 2 - 4, 'U', VC_RED);
}

#define HAND_LIFT 10

/* Overlap step: cards fan left-to-right showing each one's left corner-value
   strip. Wider for small hands, tighter for big ones so a 20-card hand fits. */
static int hand_step(unsigned char count) {
    int s;
    if (count <= 1) return 0;
    s = (304 - CARD_W) / (count - 1);
    if (s > 20) s = 20;   /* don't spread cards apart more than this */
    if (s < 10) s = 10;   /* keep at least the corner value visible */
    return s;
}

void vbmp_hand(unsigned int hx, unsigned char hy, const unsigned char *suits,
               const unsigned char *values, unsigned char count, unsigned char selected) {
    unsigned char i;
    int step = hand_step(count);
    unsigned int sx;

    for (i = 0; i < count; i++) {
        if (i == selected) continue;
        vbmp_card(hx + (unsigned int)(i * step), hy, suits[i], values[i]);
    }

    sx = hx + (unsigned int)(selected * step);
    vbmp_card(sx, (unsigned char)(hy - HAND_LIFT), suits[selected], values[selected]);
    vbmp_frame_rect(sx - 2, (unsigned char)(hy - HAND_LIFT - 2), CARD_W + 4, CARD_H + 4, VC_YELLOW);
    vbmp_frame_rect(sx - 1, (unsigned char)(hy - HAND_LIFT - 1), CARD_W + 2, CARD_H + 2, VC_YELLOW);
}
