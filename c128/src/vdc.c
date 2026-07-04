#include "vdc.h"
#include "charset_data.h"

#define VDC_CTRL (*(volatile unsigned char *)0xD600)
#define VDC_DATA (*(volatile unsigned char *)0xD601)
#define VIC_RASTER (*(unsigned char *)0xD012)

static void wait_ready(void) {
    while (!(VDC_CTRL & 0x80)) {}
}

unsigned char vdc_reg_read(unsigned char reg) {
    wait_ready();
    VDC_CTRL = reg;
    wait_ready();
    return VDC_DATA;
}

void vdc_reg_write(unsigned char reg, unsigned char value) {
    wait_ready();
    VDC_CTRL = reg;
    wait_ready();
    VDC_DATA = value;
}

void vdc_set_address(unsigned int addr) {
    vdc_reg_write(18, (unsigned char)(addr >> 8));
    vdc_reg_write(19, (unsigned char)(addr & 0xFF));
}

void vdc_data_write(unsigned char value) {
    vdc_reg_write(31, value);
}

unsigned char vdc_data_read(void) {
    return vdc_reg_read(31);
}

void vdc_reg_select_31(void) {
    wait_ready();
    VDC_CTRL = 31;
}

void vdc_data_write_selected(unsigned char value) {
    wait_ready();
    VDC_DATA = value;
    wait_ready();
}

unsigned char vdc_data_read_selected(void) {
    unsigned char v;
    wait_ready();
    v = VDC_DATA;
    wait_ready();
    return v;
}

/* The VIC-IIe keeps rastering at the normal PAL/NTSC rate even while the
   VDC drives the visible display, so its raster register still makes a
   perfectly good free frame-rate timer for pacing screen updates. */
void wait_vsync(void) {
    while (VIC_RASTER != 0) {}
    while (VIC_RASTER == 0) {}
}

static unsigned char ascii_to_screencode(char c) {
    unsigned char u = (unsigned char)c;
    if (u >= 32 && u <= 63) return u;
    if (u >= 64 && u <= 95) return u - 64;
    if (u >= 97 && u <= 122) return u - 96;
    return 32;
}

void vdc_init(void) {
    unsigned int i;

    vdc_reg_write(12, (unsigned char)(VDC_SCREEN_BASE >> 8));
    vdc_reg_write(13, (unsigned char)(VDC_SCREEN_BASE & 0xFF));
    vdc_reg_write(20, (unsigned char)(VDC_ATTR_BASE >> 8));
    vdc_reg_write(21, (unsigned char)(VDC_ATTR_BASE & 0xFF));

    /* Enable attribute (per-character color) mode, keep other mode bits. */
    vdc_reg_write(25, (unsigned char)(vdc_reg_read(25) | 0x40));

    /* Character set base address: deliberately NOT touching register 28.
       KERNAL already points it at VDC_CHARSET_BASE (0x2000) by default for
       its own font, and that register's low nibble carries unrelated bits
       (underline scan line) that must not be clobbered. We just overwrite
       the character bitmaps living at that same, already-configured address. */

    /* Background color (shared across the whole screen, low nibble). */
    vdc_reg_write(26, COL_BLACK);

    /* The KERNAL's 80-column cursor-blink IRQ also pokes VDC registers; left
       enabled, it can interleave with this multi-step upload and corrupt the
       auto-incrementing address partway through. Keep it atomic. */
    __asm__("sei");
    vdc_set_address(VDC_CHARSET_BASE);
    for (i = 0; i < 2048; i++) {
        vdc_data_write(charset_data[i]);
    }
    __asm__("cli");

    scr_clear();
}

void scr_clear(void) {
    unsigned int i;
    vdc_set_address(VDC_SCREEN_BASE);
    for (i = 0; i < (unsigned int)VDC_COLS * VDC_ROWS; i++) vdc_data_write(32);
    vdc_set_address(VDC_ATTR_BASE);
    for (i = 0; i < (unsigned int)VDC_COLS * VDC_ROWS; i++) vdc_data_write(COL_WHITE);
}

void scr_put(unsigned char x, unsigned char y, unsigned char ch, unsigned char color) {
    unsigned int off = (unsigned int)y * VDC_COLS + x;
    if (ch < 128) ch = ascii_to_screencode(ch);
    vdc_set_address(VDC_SCREEN_BASE + off);
    vdc_data_write(ch);
    vdc_set_address(VDC_ATTR_BASE + off);
    vdc_data_write(color & 0x0F);
}

void scr_puts(unsigned char x, unsigned char y, const char *s, unsigned char color) {
    unsigned int off = (unsigned int)y * VDC_COLS + x;
    const char *p;

    vdc_set_address(VDC_SCREEN_BASE + off);
    for (p = s; *p; p++) vdc_data_write(ascii_to_screencode(*p));

    vdc_set_address(VDC_ATTR_BASE + off);
    for (p = s; *p; p++) vdc_data_write(color & 0x0F);
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
    unsigned int off;

    if (ch < 128) ch = ascii_to_screencode(ch);

    for (row = 0; row < h; row++) {
        off = (unsigned int)(y + row) * VDC_COLS + x;
        vdc_set_address(VDC_SCREEN_BASE + off);
        for (col = 0; col < w; col++) vdc_data_write(ch);
        vdc_set_address(VDC_ATTR_BASE + off);
        for (col = 0; col < w; col++) vdc_data_write(color & 0x0F);
    }
}
