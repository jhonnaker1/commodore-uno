#include <coco.h>
#include "cocovid.h"

/* CoCo has no cc65-style busy-wait-only pacing concern: our program runs
   as a normal Disk Basic EXEC'd machine-language routine, so Basic's own
   60Hz clock interrupt (which increments the word at $0112, exposed as
   getTimer()) keeps ticking in the background the whole time, the same
   jiffy-clock trick every cc65 (6502) port in this repo already uses. */
void wait_vsync(void) {
    unsigned int start = getTimer();
    while (getTimer() == start) {
    }
}

static void set_ink(unsigned char color) {
    attr(color, 0, 0, 0);
}

void coco_init(void) {
    initCoCoSupport();
    width(COLS);

    /* Background palette registers are 0-7; background always stays at
       index 0 here, set to black. Foreground registers are 8-15 (index
       N here maps to register 8+N) -- see cocovid.h's comment on why
       these two aren't the same bank of colours. Values are 0-3 per
       RGB channel (paletteRGB()'s own scale, not 0-255). */
    paletteRGB(0, 0, 0, 0);
    paletteRGB(8 + COL_RED, 3, 0, 0);
    paletteRGB(8 + COL_YELLOW, 3, 3, 0);
    paletteRGB(8 + COL_GREEN, 0, 3, 0);
    paletteRGB(8 + COL_BLUE, 0, 1, 3);
    paletteRGB(8 + COL_NORMAL, 3, 3, 3);
    paletteRGB(8 + COL_WILD, 3, 0, 3);
    paletteRGB(8 + COL_SELECTED, 0, 3, 3);
    paletteRGB(8 + COL_ALERT, 3, 2, 0);

    scr_clear();
}

void scr_clear(void) {
    set_ink(COL_NORMAL);
    cls(0);
}

void scr_put(unsigned char x, unsigned char y, char ch, unsigned char color) {
    locate(x, y);
    set_ink(color);
    putchar(ch);
}

void scr_puts(unsigned char x, unsigned char y, const char *s, unsigned char color) {
    locate(x, y);
    set_ink(color);
    while (*s) {
        putchar(*s++);
    }
}

void scr_put_num(unsigned char x, unsigned char y, unsigned int n) {
    locate(x, y);
    set_ink(COL_NORMAL);
    printf("%u", n);
}

void scr_fill_rect(unsigned char x, unsigned char y, unsigned char w, unsigned char h) {
    unsigned char row, col;
    set_ink(COL_NORMAL);
    for (row = 0; row < h; row++) {
        locate(x, (unsigned char)(y + row));
        for (col = 0; col < w; col++) {
            putchar(' ');
        }
    }
}
