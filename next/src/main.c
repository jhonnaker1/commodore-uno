/* UNO for the NeXT: the Mac port's game loop (mac/src/main.c), which is
   the Amiga bitmap build's (amiga/src/main_bmp.c) with these changes:
   - the mouse can answer the colour picker and the challenge prompt as
     well as play cards;
   - queued input is dropped whenever the human is asked something, so a
     click made during a CPU turn can't act on the next question;
   - an illegal pick redraws only the hand, so the "doesn't match" message
     stays up, and a draw redraws the table, so the deck count stays right;
   - Game > New Game longjmps back to the top of the game loop from inside
     the event loop, abandoning whatever turn or prompt was in progress.
     Nothing the shared logic holds needs unwinding: game_new() rebuilds
     all of GameState, and the UI redraws from scratch.
   wait_vsync() is the AppKit event loop, and this whole loop runs inside
   the application delegate's appDidInit: (see nextgfx.h). */
#include <stdlib.h>
#include <setjmp.h>
#include <time.h>
#include "nextgfx.h"
#include "nextsound.h"
#include "input.h"
#include "cards.h"
#include "game.h"
#include "ai.h"
#include "ui.h"

static GameState g;

static jmp_buf new_game_jmp;

static void new_game_requested(void) {
    longjmp(new_game_jmp, 1);
}

static void pause_frames(unsigned int n) {
    while (n--) wait_vsync();
}

static unsigned char human_has_legal_move(GameState *g) {
    Player *p = &g->players[0];
    unsigned char i;
    for (i = 0; i < p->count; i++)
        if (is_legal(g, p->hand[i])) return 1;
    return 0;
}

static unsigned char handle_post_play_flags(GameState *g) {
    if (g->flag_win) { sfx_win(); return 1; }
    if (g->flag_skip != NONE) { ui_event_skip(g->flag_skip); sfx_skip(); pause_frames(70); }
    if (g->flag_reverse != NONE) { ui_event_reverse(g->flag_reverse); sfx_reverse(); pause_frames(60); }
    if (g->flag_draw_player != NONE) {
        ui_event_draw(g->flag_draw_player, g->flag_draw_count);
        sfx_draw_multi(g->flag_draw_count);
        pause_frames(80);
    }
    if (g->flag_uno_player != NONE) { ui_event_uno(g->flag_uno_player); sfx_uno(); pause_frames(80); }
    return 0;
}

static unsigned char human_challenge_prompt(GameState *g) {
    unsigned char sel = 0, pressed, qs;
    gfx_flush_input();
    ui_draw_challenge_prompt(g->wd4_victim, g->wd4_player, sel);
    for (;;) {
        wait_vsync();
        pressed = joy_pressed();
        qs = joy_quick_select();
        if (qs <= 1) return qs; /* clicked Yes (1) or No (0) */
        if ((pressed & IN_LEFT) || (pressed & IN_RIGHT)) {
            sel = !sel;
            ui_draw_challenge_prompt(g->wd4_victim, g->wd4_player, sel);
        }
        if (pressed & IN_FIRE) return sel;
    }
}

static unsigned char resolve_pending_wd4(GameState *g) {
    unsigned char challenged;
    if (g->wd4_victim == 0) challenged = human_challenge_prompt(g);
    else challenged = ai_should_challenge_wd4(g, g->wd4_victim);

    if (challenged) {
        unsigned char succeeded = !g->wd4_was_legal;
        resolve_wd4(g, challenged);
        ui_event_challenge_result(g->wd4_victim, g->wd4_player, succeeded);
        if (succeeded) sfx_challenge_success();
        else sfx_challenge_fail();
        ui_draw_opponents(g);
        ui_draw_hand(g, 0);
        pause_frames(90);
    } else {
        resolve_wd4(g, challenged);
    }
    ui_draw_opponents(g);
    ui_draw_table(g);
    ui_draw_hand(g, 0);
    return handle_post_play_flags(g);
}

static unsigned char after_play(GameState *g) {
    if (g->wd4_pending) return resolve_pending_wd4(g);
    return handle_post_play_flags(g);
}

static unsigned char human_pick_color(GameState *g) {
    unsigned char sel = 0, pressed, qs;
    (void)g;
    gfx_flush_input();
    ui_draw_color_picker(sel);
    for (;;) {
        wait_vsync();
        pressed = joy_pressed();
        qs = joy_quick_select();
        if (qs < 4) { ui_clear_color_picker(); return qs; } /* clicked a colour */
        if (pressed & IN_RIGHT) { sel = (sel + 1) & 3; ui_draw_color_picker(sel); }
        if (pressed & IN_LEFT) { sel = (sel + 3) & 3; ui_draw_color_picker(sel); }
        if (pressed & IN_FIRE) { ui_clear_color_picker(); return sel; }
    }
}

static unsigned char try_play(GameState *g, unsigned char idx, unsigned char *game_over) {
    Card c;
    unsigned char chosen;
    c = g->players[0].hand[idx];
    if (!is_legal(g, c)) { sfx_invalid(); ui_event_invalid(); return 0; }
    chosen = (c.color == COLOR_WILD) ? human_pick_color(g) : 0;
    play_card(g, idx, chosen);
    sfx_card_play();
    ui_draw_opponents(g);
    ui_draw_table(g);
    ui_draw_hand(g, 0);
    *game_over = after_play(g);
    return 1;
}

static unsigned char human_turn(GameState *g) {
    unsigned char cursor = 0, redraw = 1, pressed, qs, over, idx, chosen;
    Card drawn;
    gfx_flush_input();
    for (;;) {
        if (redraw) {
            ui_draw_opponents(g);
            ui_draw_table(g);
            ui_draw_hand(g, cursor);
            if (human_has_legal_move(g))
                ui_message("Your turn", "Click a card to play it, or click the deck to draw.");
            else
                ui_message("Your turn", "No card matches - click the deck (or press U) to draw.");
            redraw = 0;
        }
        wait_vsync();
        pressed = joy_pressed();
        qs = joy_quick_select();
        if (pressed & IN_RIGHT) { if (cursor + 1 < g->players[0].count) cursor++; redraw = 1; }
        if (pressed & IN_LEFT) { if (cursor > 0) cursor--; redraw = 1; }
        if (pressed & IN_UP) {
            drawn = draw_card(g, 0);
            sfx_draw();
            ui_event_drew_one(0);
            ui_draw_opponents(g);
            ui_draw_table(g); /* the deck count changed */
            ui_draw_hand(g, g->players[0].count - 1);
            pause_frames(40);
            if (is_legal(g, drawn)) {
                idx = g->players[0].count - 1;
                chosen = (drawn.color == COLOR_WILD) ? human_pick_color(g) : 0;
                play_card(g, idx, chosen);
                sfx_card_play();
                ui_draw_opponents(g);
                ui_draw_table(g);
                ui_draw_hand(g, 0);
                return after_play(g);
            }
            end_turn_no_play(g);
            return 0;
        }
        /* An illegal pick only moves the cursor: redrawing the whole turn
           would repaint "Your turn" over ui_event_invalid()'s message. */
        if (qs != IN_NONE && qs < g->players[0].count) {
            cursor = qs;
            if (try_play(g, cursor, &over)) return over;
            ui_draw_hand(g, cursor);
        } else if (pressed & IN_FIRE) {
            if (try_play(g, cursor, &over)) return over;
            ui_draw_hand(g, cursor);
        }
    }
}

static unsigned char cpu_turn(GameState *g, unsigned char idx) {
    unsigned char hand_idx, chosen;
    Card drawn, c;

    ui_draw_opponents(g);
    ui_draw_table(g);
    ui_draw_hand(g, 0);
    ui_event_thinking(idx);
    pause_frames(45);

    hand_idx = ai_choose_card(g, idx);
    if (hand_idx == NONE) {
        drawn = draw_card(g, idx);
        sfx_draw();
        ui_event_drew_one(idx);
        ui_draw_opponents(g);
        ui_draw_table(g); /* the deck count changed */
        pause_frames(40);
        if (is_legal(g, drawn)) {
            hand_idx = g->players[idx].count - 1;
            chosen = (drawn.color == COLOR_WILD) ? ai_choose_color(g, idx) : 0;
            play_card(g, hand_idx, chosen);
            sfx_card_play();
        } else {
            end_turn_no_play(g);
            return 0;
        }
    } else {
        c = g->players[idx].hand[hand_idx];
        chosen = (c.color == COLOR_WILD) ? ai_choose_color(g, idx) : 0;
        play_card(g, hand_idx, chosen);
        sfx_card_play();
    }
    ui_draw_opponents(g);
    ui_draw_table(g);
    return after_play(g);
}

static unsigned int seed_from_wait(void) {
    unsigned int counter = 0;
    for (;;) {
        wait_vsync();
        counter++;
        if (joy_pressed() & IN_FIRE) break;
    }
    return counter;
}

void uno_main(void) {
    unsigned char game_over, human_won;
    static volatile unsigned char skip_title;

    gfx_init();
    snd_init();

    if (setjmp(new_game_jmp)) skip_title = 1;
    gfx_on_new_game(new_game_requested);

    for (;;) {
        if (skip_title) {
            srand((unsigned int)time(NULL));
            skip_title = 0;
        } else {
            ui_title_screen();
            /* the clock as well: a player who always presses Return
               after the same wait would otherwise get the same deal */
            srand(seed_from_wait() ^ (unsigned int)time(NULL));
        }

        game_new(&g);
        game_over = 0;
        ui_draw_frame();
        ui_draw_opponents(&g);
        ui_draw_table(&g);
        ui_draw_hand(&g, 0);

        while (!game_over) {
            if (g.current_player == 0) game_over = human_turn(&g);
            else game_over = cpu_turn(&g, g.current_player);
        }

        human_won = (g.winner == 0) ? 1 : 0;
        ui_game_over_screen(human_won, g.winner);
        gfx_flush_input();
        for (;;) {
            wait_vsync();
            if (joy_pressed() & IN_FIRE) break;
        }
    }
}
