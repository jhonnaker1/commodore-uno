#include "vbxevid.h"
int main(void) {
    vbxe_init();
    scr_puts(0, 0, "ROW0 WHITE HELLO VBXE", COL_WHITE);
    scr_puts(0, 1, "ROW1 RED", COL_RED);
    scr_puts(0, 2, "ROW2 YELLOW", COL_YELLOW);
    scr_puts(0, 3, "ROW3 GREEN", COL_GREEN);
    scr_puts(0, 5, "ROW5 BLUE", COL_BLUE);
    scr_puts(0, 12, "ROW12 MIDDLE", COL_WHITE);
    scr_puts(0, 23, "ROW23 BOTTOM", COL_WHITE);
    for (;;) {}
    return 0;
}
