#include <conio.h>
#include "applevid.h"

/* cc65's clock()/time.h isn't implemented at all for apple2/apple2enh
   (unresolved external at link time) -- unlike the PET and Atari,
   which have an OS jiffy clock to fall back on. The Apple II does have
   a real vertical-blank status bit at $C019 (bit 7), so pace on that
   directly instead, the same style as the VIC-II/TED raster-based
   wait_vsync() on the Commodore ports. */
#define VBLBAR (*(unsigned char *)0xC019)

void apple_init(void) {
    clrscr();
}

void wait_vsync(void) {
    while ((VBLBAR & 0x80) == 0) { }
    while ((VBLBAR & 0x80) != 0) { }
}

void scr_clear(void) {
    clrscr();
}

void scr_put(unsigned char x, unsigned char y, char ch, unsigned char inverse) {
    revers(inverse);
    cputcxy(x, y, ch);
    revers(0);
}

void scr_puts(unsigned char x, unsigned char y, const char *s, unsigned char inverse) {
    revers(inverse);
    cputsxy(x, y, s);
    revers(0);
}

void scr_put_num(unsigned char x, unsigned char y, unsigned int n) {
    char buf[6];
    unsigned char i = 0;
    if (n == 0) {
        scr_put(x, y, '0', 0);
        return;
    }
    while (n > 0 && i < 5) {
        buf[i++] = (char)('0' + (n % 10));
        n /= 10;
    }
    while (i > 0) {
        i--;
        scr_put(x, y, buf[i], 0);
        x++;
    }
}

void scr_fill_rect(unsigned char x, unsigned char y, unsigned char w, unsigned char h) {
    unsigned char row, col;
    for (row = 0; row < h; row++) {
        for (col = 0; col < w; col++) {
            scr_put((unsigned char)(x + col), (unsigned char)(y + row), ' ', 0);
        }
    }
}
