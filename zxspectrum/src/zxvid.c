#include <stdio.h>
#include <arch/zx.h>
#include <input.h>
#include "zxvid.h"

/* Polling the ROM's FRAMES counter (updated by its 50Hz interrupt handler)
   would be the natural choice, but enabling interrupts crashed within a
   few frames -- the C runtime's stack apparently isn't laid out somewhere
   safe for the ROM's interrupt handler to push onto here. z88dk's own
   in_pause() busy-waits a fixed duration without touching interrupt state
   at all, which is the safer, more standard way these "bare" z88dk
   programs pace themselves. */

void zx_init(void) {
    scr_clear();
}

void wait_vsync(void) {
    in_pause(20); /* ~1 frame at 50Hz */
}

void scr_clear(void) {
    zx_cls(PAPER_WHITE);
    putchar('\x0B'); /* HOME: reset stdio's own AT cursor tracker to 0,0 */
}

static void goto_xy(unsigned char x, unsigned char y) {
    putchar('\x16');
    putchar((char)(x + 1));
    putchar((char)(y + 1));
}

void scr_put(unsigned char x, unsigned char y, char ch, unsigned char inverse) {
    goto_xy(x, y);
    if (inverse) putchar('\x14');
    putchar(ch);
    if (inverse) putchar('\x14');
}

void scr_puts(unsigned char x, unsigned char y, const char *s, unsigned char inverse) {
    goto_xy(x, y);
    if (inverse) putchar('\x14');
    while (*s) putchar(*s++);
    if (inverse) putchar('\x14');
}

void scr_put_num(unsigned char x, unsigned char y, unsigned int n) {
    goto_xy(x, y);
    printf("%u", n);
}

void scr_fill_rect(unsigned char x, unsigned char y, unsigned char w, unsigned char h) {
    unsigned char row, col;
    for (row = 0; row < h; row++) {
        goto_xy(x, (unsigned char)(y + row));
        for (col = 0; col < w; col++) putchar(' ');
    }
}
