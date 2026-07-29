/* F256 video smoke test: proves the Vicky text mode, the per-cell fg/bg
   colour matrix, and the CLUT palette -- before any game logic. Draws a
   title and a row of colour-tile "cards", then spins. */
#include "f256vid.h"

/* draw a 6x4 card tile: solid suit-coloured block with a white value char */
static void card(unsigned char x, unsigned char y, unsigned char suit, char v) {
    f256_fill(x, y, 6, 4, ' ', F256_COLOR(FC_WHITE, suit));
    f256_put(x + 1, y + 1, (unsigned char)v, F256_COLOR(FC_WHITE, suit));
    f256_put(x + 4, y + 2, (unsigned char)v, F256_COLOR(FC_WHITE, suit));
    f256_put(x + 2, y + 1, (unsigned char)v, F256_COLOR(FC_WHITE, suit));
}

int main(void) {
    __asm__("sei");
    f256_text_init();

    f256_palette(FC_BLACK, 0, 0, 0);
    f256_palette(FC_WHITE, 255, 255, 255);
    f256_palette(FC_RED, 200, 40, 40);
    f256_palette(FC_GREEN, 40, 170, 60);
    f256_palette(FC_BLUE, 70, 100, 220);
    f256_palette(FC_YELLOW, 230, 200, 40);
    f256_palette(FC_GRAY, 150, 150, 150);
    f256_palette(FC_FELT, 20, 90, 50);

    f256_clear(F256_COLOR(FC_WHITE, FC_FELT));

    f256_puts(36, 2, "U N O", F256_COLOR(FC_YELLOW, FC_FELT));
    f256_puts(29, 4, "FOENIX  F256K  EDITION", F256_COLOR(FC_WHITE, FC_FELT));

    f256_puts(8, 8, "TOP CARD", F256_COLOR(FC_GRAY, FC_FELT));
    card(8, 9, FC_RED, '5');

    f256_puts(20, 8, "YOUR HAND", F256_COLOR(FC_WHITE, FC_FELT));
    card(20, 9, FC_RED, '5');
    card(28, 9, FC_YELLOW, '7');
    card(36, 9, FC_GREEN, '2');
    card(44, 9, FC_BLUE, '9');
    card(52, 9, FC_GRAY, 'W');

    for (;;) { }
    return 0;
}
