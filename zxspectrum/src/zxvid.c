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

/* Sets ink AND paper on every call, not just ink -- confirmed empirically
   (see zxvid.h) that z88dk's console driver defaults each freshly-printed
   character's paper to white regardless of what zx_cls() filled the
   background with, which only affects untouched cells. Sending only
   \x10 (INK) left every printed character sitting on that default white
   paper; white ink text was then genuinely invisible (white on white),
   which is exactly the solid-looking block every "normal" text line
   rendered as before this was caught. */
static void set_ink(unsigned char color) {
    putchar('\x10');
    putchar((char)color);
    putchar('\x11');
    putchar(0); /* paper: always black (raw 0-7 value, not zx_cls()'s packed one) */
}

void scr_clear(void) {
    zx_cls(PAPER_BLACK);
    putchar('\x0B'); /* HOME: reset stdio's own AT cursor tracker to 0,0 */
    set_ink(COL_NORMAL);
}

static void goto_xy(unsigned char x, unsigned char y) {
    putchar('\x16');
    putchar((char)(x + 1));
    putchar((char)(y + 1));
}

void scr_put(unsigned char x, unsigned char y, char ch, unsigned char color) {
    goto_xy(x, y);
    set_ink(color);
    putchar(ch);
}

void scr_puts(unsigned char x, unsigned char y, const char *s, unsigned char color) {
    goto_xy(x, y);
    set_ink(color);
    while (*s) putchar(*s++);
}

void scr_put_num(unsigned char x, unsigned char y, unsigned int n) {
    goto_xy(x, y);
    set_ink(COL_NORMAL);
    printf("%u", n);
}

void scr_fill_rect(unsigned char x, unsigned char y, unsigned char w, unsigned char h) {
    unsigned char row, col;
    set_ink(COL_NORMAL);
    for (row = 0; row < h; row++) {
        goto_xy(x, (unsigned char)(y + row));
        for (col = 0; col < w; col++) putchar(' ');
    }
}
