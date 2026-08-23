/* Render check for the native 80-column build. Draws one screen and then
   spins, so the host can capture it.

   The capture method is worth explaining, because it is not the one the
   C64-mode build uses. That one is verified with Xemu's `-prgexit
   -screenshot`, which saves the framebuffer when the program returns to
   the READY. prompt. llvm-mos binaries do not reliably return to BASIC on
   the MEGA65, so -prgexit never fires and the emulator runs forever.

   Sending SIGTERM saves the screenshot and the ASCII screen dump just the
   same -- and that works on a program spinning in its main loop, which
   -prgexit never could. So this file does not need the "draw a state then
   return" contortion the C64-mode verify builds needed; it can simply draw
   and spin, exactly like the real game does.

   Build one screen at a time: -DSMOKE_SCREEN=0 dealt table, 1 colour
   picker, 2 picker cleared (its frame is taller than an ordinary message,
   so the rows underneath want checking), 3 a full 20-card hand, 4 an
   autoplay soak that runs the real engine for a whole game with the AI in
   every seat, redrawing each turn -- which is the only one of these that
   would catch a crash or creeping corruption rather than a layout mistake. */
#include <stdlib.h>
#include "m65native.h"
#include "game.h"
#include "ai.h"
#include "cards.h"
#include "ui.h"

#ifndef SMOKE_SCREEN
#define SMOKE_SCREEN 0
#endif

static GameState g;

int main(void)
{
    mega65_init();

    /* Fixed seed: the dealt screen is then the same every run, so a change
       in the dump means a real change rather than a different deal. */
    srand(1);
    game_new(&g);

    ui_draw_frame();
    ui_draw_opponents(&g);
    ui_draw_table(&g);

#if SMOKE_SCREEN == 4
    /* Autoplay soak: the real engine, with the AI taking every seat
       including the human's, redrawing every screen each turn until someone
       wins. The four static screens above only prove a layout; this is what
       would surface a crash, a runaway loop, or colour RAM being walked on
       after a few hundred redraws. The turn cap is a guard against an
       engine bug leaving nobody able to move -- reaching it is itself a
       failure, so the final screen says which happened. */
    {
        unsigned int turn;
        unsigned char idx, hand_idx, chosen;
        Card drawn, c;

        for (turn = 0; turn < 2000 && !g.flag_win; turn++) {
            idx = g.current_player;
            hand_idx = ai_choose_card(&g, idx);
            if (hand_idx == NONE) {
                drawn = draw_card(&g, idx);
                if (is_legal(&g, drawn)) {
                    hand_idx = (unsigned char)(g.players[idx].count - 1);
                    chosen = (drawn.color == COLOR_WILD) ? ai_choose_color(&g, idx) : 0;
                    play_card(&g, hand_idx, chosen);
                } else {
                    end_turn_no_play(&g);
                }
            } else {
                c = g.players[idx].hand[hand_idx];
                chosen = (c.color == COLOR_WILD) ? ai_choose_color(&g, idx) : 0;
                play_card(&g, hand_idx, chosen);
            }
            if (g.wd4_pending) resolve_wd4(&g, ai_should_challenge_wd4(&g, g.wd4_victim));

            ui_draw_opponents(&g);
            ui_draw_table(&g);
            ui_draw_hand(&g, 0);
        }

        if (g.flag_win) {
            ui_message("SOAK OK - GAME COMPLETED", "WINNER SHOWN BELOW");
            ui_game_over_screen(g.winner == 0, g.winner);
        } else {
            ui_message("SOAK FAILED - TURN CAP HIT", "NOBODY WON IN 2000 TURNS");
        }
        for (;;) { }
    }
#endif

#if SMOKE_SCREEN == 3
    /* Force a full hand so both rows and the 11/9 split get drawn. */
    while (g.players[0].count < HAND_VISIBLE) draw_card(&g, 0);
#endif

    ui_draw_hand(&g, 0);
    ui_message("YOUR TURN", "LEFT/RIGHT THEN FIRE, UP TO DRAW, OR PRESS A CARD'S KEY");

#if SMOKE_SCREEN == 1
    ui_draw_color_picker(1);
#elif SMOKE_SCREEN == 2
    ui_draw_color_picker(1);
    ui_clear_color_picker();
    ui_message("YOUR TURN", "LEFT/RIGHT THEN FIRE, UP TO DRAW, OR PRESS A CARD'S KEY");
#endif

    for (;;) { }
    return 0;
}
