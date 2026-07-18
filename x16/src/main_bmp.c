/* Bitmap-graphics rendering prototype for the Commander X16.
   A separate build target from the text-mode game (see the Makefile's
   `bmp` target); this only proves the framebuffer + font + card-art
   pipeline: it draws the felt table and a row of pixel-drawn card faces
   using the KERNAL GRAPH_* API. No game logic yet. */
#include "vbmp.h"
#include "cards.h"

int main(void) {
    gfx_init();

    /* felt table across the whole 320x240 screen */
    gfx_set_colors(GC_FELT, GC_FELT, GC_FELT);
    gfx_rect(0, 0, 320, 240, 1);

    /* title */
    gfx_set_colors(GC_YELLOW, GC_YELLOW, GC_FELT);
    gfx_text(132, 6, "U N O");
    gfx_set_colors(GC_WHITE, GC_WHITE, GC_FELT);
    gfx_text(56, 18, "BITMAP MODE - COMMANDER X16");

    /* draw pile (face down) and the current top card */
    gfx_set_colors(GC_WHITE, GC_WHITE, GC_FELT);
    gfx_text(64, 42, "DRAW");
    vb_card_back(60, 54);

    gfx_set_colors(GC_WHITE, GC_WHITE, GC_FELT);
    gfx_text(150, 42, "TOP CARD");
    vb_card(150, 54, 0, 5);          /* red 5 */

    /* a fanned hand of pixel-drawn cards -- overlapping, with one lifted &
       highlighted. Demonstrates how a variable (here 12) card hand fits. */
    {
        static const unsigned char hs[20] =
            {0, 3, 2, 1, 3, 0, 2, 1, 0, 3, 2, 4, 1, 0, 3, 2, 1, 4, 0, 2};
        static const unsigned char hv[20] =
            {5, 7, 2, 9, VAL_SKIP, 0, 3, VAL_REVERSE, 8, 1, VAL_DRAW2, VAL_WILD,
             4, 6, VAL_SKIP, 7, 2, VAL_WILD4, 9, VAL_REVERSE};

        gfx_set_colors(GC_WHITE, GC_WHITE, GC_FELT);
        gfx_text(8, 150, "YOUR HAND - 20 CARDS (FANNED)");
        vb_hand(8, 168, hs, hv, 20, 7);   /* card 7 (green reverse) selected */
    }

    for (;;) { }
    return 0;
}
