/* VBXE bitmap prototype: proves the framebuffer + palette + ROM-font blit +
   card-art pipeline. Draws the felt table, a title, and a row of pixel-drawn
   UNO card faces. Not the game -- a rendering prototype (its own build
   target; the text VBXE build is untouched). */
#include "vbxebmp.h"

/* card action codes (mirrors cards.h) */
#define VAL_SKIP 10
#define VAL_WILD 13

int main(void) {
    vbmp_init();

    /* palette used by the card renderer: 0 black, 1 white, 2 red, 3 green,
       4 blue, 5 yellow, 12 gray(wild), 15 shadow, 16 felt table */
    vbmp_palette(0, 0, 0, 0);
    vbmp_palette(1, 255, 255, 255);
    vbmp_palette(2, 200, 40, 40);
    vbmp_palette(3, 40, 170, 60);
    vbmp_palette(4, 70, 100, 220);
    vbmp_palette(5, 230, 200, 40);
    vbmp_palette(12, 150, 150, 150);
    vbmp_palette(15, 20, 40, 28);
    vbmp_palette(16, 20, 90, 50);

    vbmp_clear(16);

    vbmp_text(96, 8, "U N O", 5);
    vbmp_text(56, 22, "VBXE BITMAP EDITION", 1);

    vbmp_text(24, 44, "TOP CARD", 1);
    vbmp_card(24, 56, 0, 5);            /* red 5 */

    vbmp_text(150, 44, "YOUR HAND", 1);
    vbmp_card(120, 120, 0, 5);         /* red 5 */
    vbmp_card(166, 120, 3, 7);         /* blue 7 */
    vbmp_card(212, 120, 2, 2);         /* green 2 */
    vbmp_card(258, 120, 1, 9);         /* yellow 9 */
    vbmp_card(120, 56, 4, VAL_WILD);   /* wild */
    vbmp_card(166, 56, 3, VAL_SKIP);   /* blue skip */

    for (;;) { vbmp_wait_vsync(); }
    return 0;
}
