#include "vdc.h"
#include "charset_data.h"

#define VDC_CTRL (*(volatile unsigned char *)0xD600)
#define VDC_DATA (*(volatile unsigned char *)0xD601)
#define VIC_RASTER (*(unsigned char *)0xD012)
#define C128_CLKRATE (*(unsigned char *)0xD030)

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

/* Register 28's high bits (character set base) -- computed once in
   vdc_init() and re-asserted every frame below. Setting it only once at
   init still produced garbled glyphs on screen; the likely explanation is
   the same fight the VIC-IIe path has to keep winning against the
   KERNAL's IRQ over $D018 (see vic.c's wait_vsync()): the KERNAL's
   80-column cursor-blink IRQ probably re-pokes VDC register 28 back to
   its own default periodically, undoing an init-time-only write. */
static unsigned char reg28_value;

/* The VIC-IIe keeps rastering at the normal PAL/NTSC rate even while the
   VDC drives the visible display, so its raster register still makes a
   perfectly good free frame-rate timer for pacing screen updates. */
void wait_vsync(void) {
    while (VIC_RASTER != 0) {}
    while (VIC_RASTER == 0) {}
    vdc_reg_write(28, reg28_value);
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

    /* Character set base address (register 28): bits 5-7 select the 8K
       page (value = base >> 13, shifted into position), bits 0-4 are the
       underline scan line and must be preserved rather than clobbered.
       An earlier version of this driver assumed the KERNAL already leaves
       this pointed at VDC_CHARSET_BASE by default and skipped writing it
       -- that assumption was never actually verified against real VDC
       behavior. Actual value is re-asserted every frame in wait_vsync(). */
    reg28_value = (unsigned char)((vdc_reg_read(28) & 0x1F) |
                                   ((VDC_CHARSET_BASE >> 13) << 5));
    vdc_reg_write(28, reg28_value);

    /* Background color (shared across the whole screen, low nibble). */
    vdc_reg_write(26, COL_BLACK);

    /* The KERNAL's 80-column cursor-blink IRQ also pokes VDC registers; left
       enabled, it can interleave with this multi-step upload and corrupt the
       auto-incrementing address partway through. Keep it atomic. */
    __asm__("sei");
    vdc_set_address(VDC_CHARSET_BASE);
    /* The VDC's character generator memory is organized in 16-byte slots
       per glyph even for an 8-scanline-tall font (register 9 selects how
       many of those 16 bytes are actually displayed) -- confirmed via the
       remote monitor: register 28/9 and the uploaded data were all
       correct, yet glyphs still rendered wrong, consistently, until this
       padding was added. charset_data is tightly packed at 8 bytes/glyph,
       so pad every glyph out to 16 on the way in rather than change that
       shared, VIC-II-format source data. */
    for (i = 0; i < 256; i++) {
        unsigned char k;
        for (k = 0; k < 8; k++) vdc_data_write(charset_data[i * 8 + k]);
        for (k = 0; k < 8; k++) vdc_data_write(0);
    }
    __asm__("cli");

    /* This build is entirely VDC-driven, so the VIC-IIe's own picture
       being unwatchable in 2MHz mode (well-documented: the VIC-II can't
       fetch character/color data coherently at 2x speed, scrambling
       whatever it displays) doesn't matter -- nobody's looking at it.
       What did need checking was whether that scrambling extends to the
       raster *register* wait_vsync() reads for pacing, since a broken
       counter would break frame timing project-wide, not just the
       picture: confirmed empirically (a standalone test sampling D012
       before/after) that it keeps incrementing normally and wrapping
       0-311 either way. Safe to leave on for the whole session -- no
       matching "back to 1MHz" anywhere, since C128 BASIC itself runs
       fine at 2MHz and this program never returns to it.
       Bit 0 of $D030 (undefined/reserved on a real C64, which is why
       this register is C128-specific) is the only bit this touches;
       the rest are left alone. */
    C128_CLKRATE |= 0x01;

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
