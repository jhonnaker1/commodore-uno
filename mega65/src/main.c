#include <stdlib.h>
#include <ascii_charmap.h>
#include "mega65vid.h"
#include "mega65snd.h"
#include "mega65spr.h"
#include "input.h"
#include "cards.h"
#include "game.h"
#include "ai.h"
#include "ui.h"

static GameState g;

static void pause_frames(unsigned int n) {
    while (n--) wait_vsync();
}

/* Card layout constants shared with ui.c (hand card i box at
   x=1+(i%10)*4, y=15/20; the discard "TOP CARD" box at x=19,y=5). */
#define TOP_COL 19
#define TOP_ROW 5
#define TOSS_STEPS 16

/* Suit index (COLOR_RED..COLOR_BLUE, or a chosen colour for wilds) ->
   MEGA65 palette index for the sprite. */
static unsigned char suit_spr_color(unsigned char suit) {
    switch (suit) {
        case COLOR_RED: return COL_RED;
        case COLOR_YELLOW: return COL_YELLOW;
        case COLOR_GREEN: return COL_GREEN;
        case COLOR_BLUE: return COL_BLUE;
        default: return COL_LTGRAY;
    }
}

/* Glides the hardware-sprite card from a hand/opponent cell to the discard
   pile. The caller redraws the table/hand right after, so the sprite just
   hides itself at the end. */
static void animate_toss(unsigned char from_col, unsigned char from_row, unsigned char color) {
    unsigned char step;
    int x0 = from_col, y0 = from_row, dx = TOP_COL - x0, dy = TOP_ROW - y0;

    spr_show(color, from_col, from_row);
    for (step = 1; step <= TOSS_STEPS; step++) {
        unsigned char cx = (unsigned char)(x0 + (dx * (int)step) / TOSS_STEPS);
        unsigned char cy = (unsigned char)(y0 + (dy * (int)step) / TOSS_STEPS);
        spr_move(cx, cy);
        wait_vsync();
    }
    spr_hide();
}

/* Screen cell of hand card `idx` (matches ui.c's ui_draw_hand layout). */
static unsigned char hand_col(unsigned char idx) { return (unsigned char)(1 + (idx % 10) * 4); }
static unsigned char hand_row(unsigned char idx) { return (unsigned char)(idx < 10 ? 15 : 20); }
/* Screen cell of opponent `idx` (1-3), matching ui.c's ui_draw_opponents. */
static unsigned char opp_col(unsigned char idx) { return (unsigned char)(2 + (idx - 1) * 13 + 2); }
#define OPP_ROW 2

/* Returns 1 if the game just ended. */
static unsigned char handle_post_play_flags(GameState *g) {
    if (g->flag_win) {
        sfx_win();
        return 1;
    }
    if (g->flag_skip != NONE) {
        wait_vsync();
        ui_event_skip(g->flag_skip);
        sfx_skip();
        pause_frames(70);
    }
    if (g->flag_reverse != NONE) {
        wait_vsync();
        ui_event_reverse(g->flag_reverse);
        sfx_reverse();
        pause_frames(60);
    }
    if (g->flag_draw_player != NONE) {
        wait_vsync();
        ui_event_draw(g->flag_draw_player, g->flag_draw_count);
        sfx_draw_multi(g->flag_draw_count);
        pause_frames(80);
    }
    if (g->flag_uno_player != NONE) {
        wait_vsync();
        ui_event_uno(g->flag_uno_player);
        sfx_uno();
        pause_frames(80);
    }
    return 0;
}

static unsigned char human_challenge_prompt(GameState *g) {
    unsigned char sel = 0;
    unsigned char pressed;
    wait_vsync();
    ui_draw_challenge_prompt(g->wd4_victim, g->wd4_player, sel);
    for (;;) {
        wait_vsync();
        pressed = joy_pressed();
        if ((pressed & IN_LEFT) || (pressed & IN_RIGHT)) {
            sel = !sel;
            ui_draw_challenge_prompt(g->wd4_victim, g->wd4_player, sel);
        }
        if (pressed & IN_FIRE) {
            return sel;
        }
    }
}

/* Handles a pending Wild Draw Four challenge decision (human prompt or AI
   heuristic), applies the outcome, and folds in the usual post-play messages. */
static unsigned char resolve_pending_wd4(GameState *g) {
    unsigned char challenged;

    if (g->wd4_victim == 0) {
        challenged = human_challenge_prompt(g);
    } else {
        challenged = ai_should_challenge_wd4(g, g->wd4_victim);
    }

    if (challenged) {
        unsigned char succeeded = !g->wd4_was_legal;
        resolve_wd4(g, challenged);
        wait_vsync();
        ui_event_challenge_result(g->wd4_victim, g->wd4_player, succeeded);
        if (succeeded) {
            sfx_challenge_success();
        } else {
            sfx_challenge_fail();
        }
        ui_draw_opponents(g);
        ui_draw_hand(g, 0);
        pause_frames(90);
    } else {
        resolve_wd4(g, challenged);
    }

    wait_vsync();
    ui_draw_opponents(g);
    ui_draw_table(g);
    ui_draw_hand(g, 0);
    return handle_post_play_flags(g);
}

/* Routes to the Wild Draw Four challenge flow if one is pending, otherwise
   the normal post-play messages. */
static unsigned char after_play(GameState *g) {
    if (g->wd4_pending) return resolve_pending_wd4(g);
    return handle_post_play_flags(g);
}

static unsigned char human_pick_color(GameState *g) {
    unsigned char sel = 0;
    unsigned char pressed;
    (void)g;
    wait_vsync();
    ui_draw_color_picker(sel);
    for (;;) {
        wait_vsync();
        pressed = joy_pressed();
        if (pressed & IN_RIGHT) {
            sel = (sel + 1) & 3;
            ui_draw_color_picker(sel);
        }
        if (pressed & IN_LEFT) {
            sel = (sel + 3) & 3;
            ui_draw_color_picker(sel);
        }
        if (pressed & IN_FIRE) {
            ui_clear_color_picker();
            return sel;
        }
    }
}

/* Attempts to play the given hand slot. Returns 1 if it was legal and played
   (with *game_over set to whatever after_play() returns), 0 if illegal. */
static unsigned char try_play(GameState *g, unsigned char idx, unsigned char *game_over) {
    Card c;
    unsigned char chosen;

    c = g->players[0].hand[idx];
    if (!is_legal(g, c)) {
        sfx_invalid();
        ui_event_invalid();
        return 0;
    }
    chosen = (c.color == COLOR_WILD) ? human_pick_color(g) : 0;
    {
        unsigned char fc = hand_col(idx), fr = hand_row(idx);
        unsigned char scol = suit_spr_color(c.color == COLOR_WILD ? chosen : c.color);
        play_card(g, idx, chosen);
        sfx_card_play();
        animate_toss(fc, fr, scol);   /* glide the played card to the pile */
    }
    wait_vsync();
    ui_draw_opponents(g);
    ui_draw_table(g);
    ui_draw_hand(g, 0);
    *game_over = after_play(g);
    return 1;
}

/* Returns 1 if the game ended during this turn. */
static unsigned char human_turn(GameState *g) {
    unsigned char cursor = 0;
    unsigned char redraw = 1;
    unsigned char pressed;
    unsigned char qs;
    unsigned char over;
    Card drawn;
    unsigned char chosen;
    unsigned char idx;

    for (;;) {
        if (redraw) {
            wait_vsync();
            ui_draw_opponents(g);
            ui_draw_table(g);
            ui_draw_hand(g, cursor);
            ui_message("YOUR TURN", "L/R/FIRE, UP:DRAW, OR A CARD'S KEY");
            redraw = 0;
        }
        wait_vsync();
        pressed = joy_pressed();
        qs = joy_quick_select();
        if (pressed & IN_RIGHT) {
            if (cursor + 1 < g->players[0].count) cursor++;
            redraw = 1;
        }
        if (pressed & IN_LEFT) {
            if (cursor > 0) cursor--;
            redraw = 1;
        }
        if (pressed & IN_UP) {
            drawn = draw_card(g, 0);
            sfx_draw();
            wait_vsync();
            ui_event_drew_one(0);
            ui_draw_opponents(g);
            ui_draw_hand(g, g->players[0].count - 1);
            pause_frames(50);
            if (is_legal(g, drawn)) {
                idx = g->players[0].count - 1;
                chosen = (drawn.color == COLOR_WILD) ? human_pick_color(g) : 0;
                {
                    unsigned char fc = hand_col(idx), fr = hand_row(idx);
                    unsigned char scol = suit_spr_color(drawn.color == COLOR_WILD ? chosen : drawn.color);
                    play_card(g, idx, chosen);
                    sfx_card_play();
                    animate_toss(fc, fr, scol);
                }
                wait_vsync();
                ui_draw_opponents(g);
                ui_draw_table(g);
                ui_draw_hand(g, 0);
                return after_play(g);
            }
            end_turn_no_play(g);
            return 0;
        }
        if (qs != IN_NONE && qs < g->players[0].count) {
            cursor = qs;
            if (try_play(g, cursor, &over)) return over;
            redraw = 1;
        } else if (pressed & IN_FIRE) {
            if (try_play(g, cursor, &over)) return over;
        }
    }
}

/* Returns 1 if the game ended during this turn. */
static unsigned char cpu_turn(GameState *g, unsigned char idx) {
    unsigned char hand_idx;
    unsigned char chosen;
    Card drawn;
    Card c;

    wait_vsync();
    ui_draw_opponents(g);
    ui_draw_table(g);
    ui_draw_hand(g, 0);
    ui_event_thinking(idx);
    pause_frames(50);

    hand_idx = ai_choose_card(g, idx);
    if (hand_idx == NONE) {
        drawn = draw_card(g, idx);
        sfx_draw();
        wait_vsync();
        ui_event_drew_one(idx);
        ui_draw_opponents(g);
        pause_frames(60);
        if (is_legal(g, drawn)) {
            hand_idx = g->players[idx].count - 1;
            chosen = (drawn.color == COLOR_WILD) ? ai_choose_color(g, idx) : 0;
            play_card(g, hand_idx, chosen);
            sfx_card_play();
            animate_toss(opp_col(idx), OPP_ROW,
                         suit_spr_color(drawn.color == COLOR_WILD ? chosen : drawn.color));
        } else {
            end_turn_no_play(g);
            return 0;
        }
    } else {
        c = g->players[idx].hand[hand_idx];
        chosen = (c.color == COLOR_WILD) ? ai_choose_color(g, idx) : 0;
        play_card(g, hand_idx, chosen);
        sfx_card_play();
        animate_toss(opp_col(idx), OPP_ROW,
                     suit_spr_color(c.color == COLOR_WILD ? chosen : c.color));
    }

    wait_vsync();
    ui_draw_opponents(g);
    ui_draw_table(g);
    return after_play(g);
}

static unsigned int seed_from_wait(void) {
    unsigned int counter = 0;
    unsigned char pressed;
    for (;;) {
        wait_vsync();
        counter++;
        pressed = joy_pressed();
        if (pressed & IN_FIRE) break;
    }
    return counter;
}

int main(void) {
    unsigned char game_over;
    unsigned char human_won;

    mega65_init();
    snd_init();
    input_init();
    spr_init();

    for (;;) {
        ui_title_screen();
        srand(seed_from_wait());

        game_new(&g);
        game_over = 0;
        ui_draw_frame();

        while (!game_over) {
            if (g.current_player == 0) {
                game_over = human_turn(&g);
            } else {
                game_over = cpu_turn(&g, g.current_player);
            }
        }

        human_won = (g.winner == 0) ? 1 : 0;
        ui_game_over_screen(human_won, g.winner);

        for (;;) {
            wait_vsync();
            if (joy_pressed() & IN_FIRE) break;
        }
    }
    return 0;
}
