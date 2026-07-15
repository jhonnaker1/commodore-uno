#include "vbxevid.h"

/* Minimal standalone display smoke-test for the VBXE driver (not part of
   the game) -- confirms the overlay initializes, colored text renders,
   and the driver's tile colors are usable directly. */
int main(void) {
    vbxe_init();
    scr_puts(0, 0, "VBXE DRIVER OK - HELLO WORLD", COL_WHITE);
    scr_puts(0, 1, "RED", COL_RED);
    scr_puts(0, 2, "YELLOW", COL_YELLOW);
    scr_puts(0, 3, "GREEN", COL_GREEN);
    scr_puts(0, 4, "BLUE", COL_BLUE);
    scr_fill_rect(0, 6, 3, 3, ' ', TILE_RED);
    scr_put(1, 7, 'R', TILE_RED);
    scr_fill_rect(4, 6, 3, 3, ' ', TILE_YELLOW);
    scr_put(5, 7, 'Y', TILE_YELLOW);
    scr_fill_rect(8, 6, 3, 3, ' ', TILE_GREEN);
    scr_put(9, 7, 'G', TILE_GREEN);
    scr_fill_rect(12, 6, 3, 3, ' ', TILE_BLUE);
    scr_put(13, 7, 'B', TILE_BLUE);
    for (;;) {
        wait_vsync();
    }
    return 0;
}
