#include "f256vid.h"

#define IOCTRL (*(volatile unsigned char *)0x0001)   /* MMU_IO_CTRL: 0=regs 2=char 3=color */
#define VKY_MCR (*(volatile unsigned char *)0xD000)   /* master control (I/O page 0) */
#define VKY_BG_B (*(volatile unsigned char *)0xD00D)
#define VKY_BG_G (*(volatile unsigned char *)0xD00E)
#define VKY_BG_R (*(volatile unsigned char *)0xD00F)
#define FG_LUT ((volatile unsigned char *)0xD800)     /* 16 entries, 4 bytes BGRx */
#define BG_LUT ((volatile unsigned char *)0xD840)
#define MATRIX ((volatile unsigned char *)0xC000)     /* char (page 2) / colour (page 3) */

void f256_text_init(void) {
    IOCTRL = 0;
    VKY_MCR = 0x01;             /* text mode enable (bit 0), no graphics layers */
    VKY_BG_B = 0; VKY_BG_G = 0; VKY_BG_R = 0;
}

void wait_vsync(void) {
    volatile unsigned int i;
    for (i = 0; i < 2500; i++) { }
}

void f256_palette(unsigned char idx, unsigned char r, unsigned char g, unsigned char b) {
    unsigned int o = (unsigned int)idx * 4;
    IOCTRL = 0;
    FG_LUT[o] = b; FG_LUT[o + 1] = g; FG_LUT[o + 2] = r; FG_LUT[o + 3] = 0;
    BG_LUT[o] = b; BG_LUT[o + 1] = g; BG_LUT[o + 2] = r; BG_LUT[o + 3] = 0;
}

/* Matrix writes page the I/O window ($0001) to 2 (chars) or 3 (colours).
   The FoenixMCP kernel's IRQ handler (which fills the keyboard event queue)
   expects I/O page 0, so an interrupt landing mid-write while the page is
   2/3 would read matrix RAM instead of its registers. Bracket each batch
   with sei/cli and always leave the page at 0. */

void f256_clear(unsigned char color) {
    unsigned int n = (unsigned int)F256_COLS * F256_ROWS;
    unsigned int i;
    __asm__("sei");
    IOCTRL = 2;
    for (i = 0; i < n; i++) MATRIX[i] = ' ';
    IOCTRL = 3;
    for (i = 0; i < n; i++) MATRIX[i] = color;
    IOCTRL = 0;
    __asm__("cli");
}

void f256_put(unsigned char x, unsigned char y, unsigned char ch, unsigned char color) {
    unsigned int off = (unsigned int)y * F256_COLS + x;
    __asm__("sei");
    IOCTRL = 2; MATRIX[off] = ch;
    IOCTRL = 3; MATRIX[off] = color;
    IOCTRL = 0;
    __asm__("cli");
}

void f256_puts(unsigned char x, unsigned char y, const char *s, unsigned char color) {
    unsigned int off = (unsigned int)y * F256_COLS + x;
    const char *p;
    __asm__("sei");
    IOCTRL = 2;
    { unsigned int o = off; for (p = s; *p; p++) MATRIX[o++] = (unsigned char)*p; }
    IOCTRL = 3;
    { unsigned int o = off; for (p = s; *p; p++) MATRIX[o++] = color; }
    IOCTRL = 0;
    __asm__("cli");
}

void f256_fill(unsigned char x, unsigned char y, unsigned char w, unsigned char h,
               unsigned char ch, unsigned char color) {
    unsigned char row, col;
    unsigned int base;
    __asm__("sei");
    IOCTRL = 2;
    for (row = 0; row < h; row++) {
        base = (unsigned int)(y + row) * F256_COLS + x;
        for (col = 0; col < w; col++) MATRIX[base + col] = ch;
    }
    IOCTRL = 3;
    for (row = 0; row < h; row++) {
        base = (unsigned int)(y + row) * F256_COLS + x;
        for (col = 0; col < w; col++) MATRIX[base + col] = color;
    }
    IOCTRL = 0;
    __asm__("cli");
}
