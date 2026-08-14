/* Render check: draws the real title screen through ui.c, then dumps the
   CGA text buffer to a file as plain text plus an attribute map, and exits.
   Not part of the game -- `make smoke`.

   Every other port in this repo could only be checked by screenshotting the
   emulator and looking. DOS can do better: the program writes a file, the
   host reads it, and layout and colour become things a script can assert on
   rather than things a human has to eyeball. */
#include <stdio.h>
#include <stdlib.h>
#include <dos.h>
#include "cgavid.h"
#include "game.h"
#include "ui.h"

static unsigned char far *vram = (unsigned char far *)MK_FP(0xB800, 0x0000);
static GameState g;

static void dump(FILE *f, const char *title)
{
    int x, y;

    fprintf(f, "=== %s ===\n", title);
    for (y = 0; y < ROWS; y++) {
        for (x = 0; x < COLS; x++) {
            unsigned char ch = vram[(y * COLS + x) * 2];
            fputc((ch >= 32 && ch < 127) ? ch : '.', f);
        }
        fputc('\n', f);
    }

    /* One hex digit per cell for the background nibble -- enough to see the
       card tiles, and to prove the bright backgrounds that disabling blink
       is supposed to unlock actually took. */
    fprintf(f, "--- %s background nibbles ---\n", title);
    for (y = 0; y < ROWS; y++) {
        for (x = 0; x < COLS; x++) {
            unsigned char at = vram[(y * COLS + x) * 2 + 1];
            fputc("0123456789ABCDEF"[(at >> 4) & 0x0F], f);
        }
        fputc('\n', f);
    }
}

int main(void)
{
    FILE *f;

    cga_init();
    f = fopen("SCREEN.TXT", "w");
    if (f == NULL) { cga_shutdown(); return 1; }

    ui_title_screen();
    dump(f, "TITLE");

    /* A fixed seed, so the dealt screen is the same every run and a
       difference in the dump means a real change rather than a new deal. */
    srand(1);
    game_new(&g);
    ui_draw_frame();
    ui_draw_opponents(&g);
    ui_draw_table(&g);
    ui_draw_hand(&g, 0);
    ui_message("YOUR TURN", "PICK A CARD AND PLAY IT");
    dump(f, "TABLE");

    fclose(f);
    cga_shutdown();
    return 0;
}
