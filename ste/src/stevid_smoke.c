/* ST/STE video smoke test: proves the 4-bitplane framebuffer, the STE
   palette encoding, the embedded font, and the card art -- before any game
   logic is wired up. Draws the felt table, a title, and a row of card faces,
   then waits for a keypress. */
#include <tos.h>
#include "stevid.h"

#define VAL_SKIP 10
#define VAL_WILD 13

static void palette_init(void) {
    ste_palette(SC_BLACK, 0, 0, 0);
    ste_palette(SC_WHITE, 15, 15, 15);
    ste_palette(SC_RED, 13, 2, 2);
    ste_palette(SC_GREEN, 2, 11, 4);
    ste_palette(SC_BLUE, 4, 6, 14);
    ste_palette(SC_YELLOW, 14, 12, 2);
    ste_palette(SC_GRAY, 9, 9, 9);
    ste_palette(SC_SHADOW, 1, 3, 2);
    ste_palette(SC_FELT, 1, 6, 3);
}

int main(void) {
    ste_init();
    palette_init();
    ste_clear(SC_FELT);

    ste_text(120, 8, "U N O", SC_YELLOW);
    ste_text(72, 22, ste_is_ste() ? "ATARI STE DETECTED" : "ATARI ST DETECTED", SC_WHITE);

    ste_text(16, 44, "TOP CARD", SC_GRAY);
    ste_card(16, 56, 0, 5);              /* red 5 */

    ste_text(96, 44, "DRAW", SC_GRAY);
    ste_card_back(96, 56);

    ste_text(160, 44, "CARDS", SC_GRAY);
    ste_card(160, 56, 3, VAL_SKIP);      /* blue skip */
    ste_card(210, 56, 4, VAL_WILD);      /* wild */
    ste_card(260, 56, 1, 7);             /* yellow 7 */

    ste_text(16, 120, "FANNED HAND:", SC_WHITE);
    {
        static const unsigned char hs[6] = {0, 1, 2, 3, 0, 4};
        static const unsigned char hv[6] = {5, 7, 2, 9, VAL_SKIP, VAL_WILD};
        ste_hand(16, 145, hs, hv, 6, 2);
    }

    Cconin();                            /* wait for a key */
    ste_shutdown();
    return 0;
}
