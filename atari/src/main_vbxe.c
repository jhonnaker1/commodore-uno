#include <stdlib.h>
#include "vbxevid.h"
#include "atarisound.h"
#include "input.h"
#include "cards.h"
#include "game.h"
#include "ai.h"
#include "ui.h"
#include "ui_vbxe.h"

static GameState g;

static void pause_frames(unsigned int n) {
    while (n--) wait_vsync();
}

/* Animates a small solid-colored block "flying" between two hand/table
   slots, then erasing itself -- the redraw-based equivalent of the C64
   port's hardware sprite toss (VBXE has no hardware sprites), moved one
   character cell at a time rather than pixel-by-pixel. Since the caller
   always redraws the affected areas (ui_draw_table/ui_draw_hand) right
   after calling this, the animation only needs to erase its OWN previous
   frame as it moves, not restore whatever was under it originally. */
#define TOSS_STEPS 8
static void animate_toss_to(unsigned char from_col, unsigned char from_row,
                             unsigned char to_col, unsigned char to_row, unsigned char color) {
    unsigned char step;
    unsigned char px = 255, py = 255;
    int x0 = from_col, y0 = from_row, x1 = to_col, y1 = to_row, dx = x1 - x0, dy = y1 - y0;

    for (step = 1; step <= TOSS_STEPS; step++) {
        unsigned char cx = (unsigned char)(x0 + (dx * (int)step) / TOSS_STEPS);
        unsigned char cy = (unsigned char)(y0 + (dy * (int)step) / TOSS_STEPS);
        if (px != 255) scr_fill_rect(px, py, 2, 1, ' ', COL_BLACK);
        scr_fill_rect(cx, cy, 2, 1, ' ', color);
        px = cx;
        py = cy;
        wait_vsync();
    }
    scr_fill_rect(px, py, 2, 1, ' ', COL_BLACK);
}

/* Convenience wrapper: toss from a slot to wherever the top-of-discard card
   currently sits, for the common "card played" case. */
static void animate_toss(unsigned char from_col, unsigned char from_row, unsigned char color) {
    unsigned char to_col, to_row;
    ui_top_card_pos(&to_col, &to_row);
    animate_toss_to(from_col, from_row, to_col, to_row, color);
}

/* Deals the human's hand out visually, one card at a time, animating each
   from the draw pile into its hand slot. game_new() has already dealt the
   real data; this only controls how it's revealed on screen. */
static void animate_deal(GameState *g) {
    unsigned char full_count = g->players[0].count;
    unsigned char i, col, row, dcol, drow;

    ui_draw_pile_pos(&dcol, &drow);

    for (i = 0; i < full_count; i++) {
        ui_hand_card_pos(i, &col, &row);
        animate_toss_to(dcol, drow, col, row, COL_CYAN);
        sfx_draw();
        g->players[0].count = (unsigned char)(i + 1);
        wait_vsync();
        ui_draw_hand(g, 0);
        pause_frames(4);
    }
    g->players[0].count = full_count;
}

/* True if the human has at least one card that can be legally played on the
   current top card -- used to swap the turn prompt to a "you must draw" hint
   when the whole hand is dimmed/unplayable. */
static unsigned char human_has_legal_move(GameState *g) {
    Player *p = &g->players[0];
    unsigned char i;
    for (i = 0; i < p->count; i++) {
        if (is_legal(g, p->hand[i])) return 1;
    }
    return 0;
}

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
    unsigned char chosen, col, row;

    c = g->players[0].hand[idx];
    if (!is_legal(g, c)) {
        sfx_invalid();
        ui_event_invalid();
        return 0;
    }
    chosen = (c.color == COLOR_WILD) ? human_pick_color(g) : 0;
    ui_hand_card_pos(idx, &col, &row);
    play_card(g, idx, chosen);
    sfx_card_play();
    animate_toss(col, row, ui_suit_color(c.color == COLOR_WILD ? chosen : c.color));
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
    unsigned char blink_counter = 0;
    unsigned char blink_state = 1;
    unsigned char col, row;

    for (;;) {
        if (redraw) {
            wait_vsync();
            ui_draw_opponents(g);
            ui_draw_table(g);
            ui_draw_hand(g, cursor);
            if (human_has_legal_move(g)) {
                ui_message("YOUR TURN", "L/R,FIRE,UP,OR A KEY TO PLAY");
            } else {
                ui_message("YOUR TURN", "NO PLAYABLE CARD - PRESS UP TO DRAW");
            }
            blink_state = 1;
            blink_counter = 0;
            redraw = 0;
        }
        wait_vsync();
        if (++blink_counter >= 25) {
            blink_counter = 0;
            blink_state = !blink_state;
            ui_blink_cursor(cursor, blink_state);
        }
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
                ui_draw_pile_pos(&col, &row);
                play_card(g, idx, chosen);
                sfx_card_play();
                animate_toss(col, row, ui_suit_color(drawn.color == COLOR_WILD ? chosen : drawn.color));
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
    unsigned char col, row;

    wait_vsync();
    ui_draw_opponents(g);
    ui_draw_table(g);
    ui_draw_hand(g, 0);
    ui_event_thinking(idx);
    pause_frames(50);

    ui_opponent_pos(idx, &col, &row);

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
            animate_toss(col, row, ui_suit_color(drawn.color == COLOR_WILD ? chosen : drawn.color));
        } else {
            end_turn_no_play(g);
            return 0;
        }
    } else {
        c = g->players[idx].hand[hand_idx];
        chosen = (c.color == COLOR_WILD) ? ai_choose_color(g, idx) : 0;
        play_card(g, hand_idx, chosen);
        sfx_card_play();
        animate_toss(col, row, ui_suit_color(c.color == COLOR_WILD ? chosen : c.color));
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

    vbxe_init();
    snd_init();

    for (;;) {
        ui_title_screen();
        srand(seed_from_wait());

        game_new(&g);
        game_over = 0;
        ui_draw_frame();
        ui_draw_opponents(&g);
        ui_draw_table(&g);
        animate_deal(&g);

        while (!game_over) {
            if (g.current_player == 0) {
                game_over = human_turn(&g);
            } else {
                game_over = cpu_turn(&g, g.current_player);
            }
        }

        human_won = (g.winner == 0) ? 1 : 0;
        ui_game_over_screen(human_won, g.winner);
        if (human_won) {
            unsigned char f;
            for (f = 0; f < 32; f++) {
                ui_win_flourish_step(f);
                pause_frames(4);
            }
        }

        for (;;) {
            wait_vsync();
            if (joy_pressed() & IN_FIRE) break;
        }
    }
    return 0;
}
