/* Render check for the graphics builds: draws the title screen and a dealt
   table, dumping the framebuffer after each so the host can turn them into
   PNGs. `make run-smoke-ega` / `make run-smoke-vga`.

   The CGA build can dump its screen as readable text, because a text mode
   *is* characters. A bitmap cannot, so this writes raw pixels instead and
   tools/dumptopng.py does the decoding. Same idea either way: the port
   reports what it actually drew, rather than being screenshotted. */
#include <stdlib.h>
#include "gfx.h"
#include "game.h"
#include "ui.h"

static GameState g;

int main(void)
{
    gfx_init();

    ui_title_screen();
    gfx_dump("TITLE.RAW");

    /* Fixed seed: the dealt screen is then the same every run, so a change
       in the dump means a real change rather than a different deal. */
    srand(1);
    game_new(&g);
    ui_draw_frame();
    ui_draw_opponents(&g);
    ui_draw_table(&g);
    ui_draw_hand(&g, 0);
    ui_message("YOUR TURN", "PICK A CARD AND PLAY IT");
    gfx_dump("TABLE.RAW");

    /* The wild colour picker, which is the one screen a player only reaches
       by holding a wild card -- easy to leave untested. */
    ui_draw_color_picker(1);
    gfx_dump("PICKER.RAW");

    /* And again after clearing it: the frame is taller than an ordinary
       message, and its bottom rows used to survive as a stray white line
       because nothing else covered them. */
    ui_clear_color_picker();
    ui_message("YOUR TURN", "PICK A CARD AND PLAY IT");
    gfx_dump("CLEARED.RAW");

    gfx_shutdown();
    return 0;
}
