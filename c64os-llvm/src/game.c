#include "game.h"

static void advance_turn(GameState *g) {
    signed char next = (signed char)g->current_player + g->direction;
    if (next < 0) next += NUM_PLAYERS;
    if (next >= NUM_PLAYERS) next -= NUM_PLAYERS;
    g->current_player = (unsigned char)next;
}

static void reshuffle_if_needed(GameState *g) {
    unsigned char i, keep_top;
    Card top;
    if (g->draw_count > 0) return;
    if (g->discard_count <= 1) return; /* nothing to reshuffle from */

    top = g->discard_pile[g->discard_count - 1];
    keep_top = g->discard_count - 1;
    for (i = 0; i < keep_top; i++) {
        g->draw_pile[i] = g->discard_pile[i];
    }
    g->draw_count = keep_top;
    deck_shuffle(g->draw_pile, g->draw_count);

    g->discard_pile[0] = top;
    g->discard_count = 1;
}

void clear_flags(GameState *g) {
    g->flag_win = 0;
    g->flag_uno_player = NONE;
    g->flag_skip = NONE;
    g->flag_reverse = NONE;
    g->flag_draw_player = NONE;
    g->flag_draw_count = 0;
}

Card draw_card(GameState *g, unsigned char player_idx) {
    Card c;
    Player *p = &g->players[player_idx];

    reshuffle_if_needed(g);
    if (g->draw_count == 0) {
        c.color = COLOR_WILD;
        c.value = VAL_WILD;
        return c;
    }
    c = g->draw_pile[g->draw_count - 1];
    g->draw_count--;
    if (p->count < MAX_HAND) {
        p->hand[p->count] = c;
        p->count++;
    }
    return c;
}

static void deal_start_card(GameState *g, unsigned char idx, Card c) {
    (void)idx;
    g->discard_pile[0] = c;
    g->discard_count = 1;
    g->top_color = c.color;
    g->top_value = c.value;
    g->top_card = c;
}

void game_new(GameState *g) {
    Card rejects[10];
    unsigned char nrej = 0;
    unsigned char i, round, p;

    deck_build(g->draw_pile);
    g->draw_count = DECK_SIZE;
    deck_shuffle(g->draw_pile, g->draw_count);

    for (p = 0; p < NUM_PLAYERS; p++) {
        g->players[p].count = 0;
        g->players[p].is_human = (p == 0) ? 1 : 0;
    }

    for (round = 0; round < 7; round++) {
        for (p = 0; p < NUM_PLAYERS; p++) {
            draw_card(g, p);
        }
    }

    g->discard_count = 0;
    for (;;) {
        Card c;
        c = g->draw_pile[g->draw_count - 1];
        g->draw_count--;
        if (c.value <= 9) {
            deal_start_card(g, 0, c);
            break;
        }
        rejects[nrej++] = c;
    }
    for (i = 0; i < nrej; i++) {
        g->draw_pile[g->draw_count] = rejects[i];
        g->draw_count++;
    }

    g->current_player = 0;
    g->direction = 1;
    g->wd4_pending = 0;
    clear_flags(g);
}

void end_turn_no_play(GameState *g) {
    clear_flags(g);
    advance_turn(g);
}

unsigned char is_legal(GameState *g, Card c) {
    if (c.color == COLOR_WILD) return 1;
    if (c.color == g->top_color) return 1;
    if (c.value == g->top_value) return 1;
    return 0;
}

static void remove_from_hand(Player *p, unsigned char idx) {
    unsigned char i;
    for (i = idx; i + 1 < p->count; i++) {
        p->hand[i] = p->hand[i + 1];
    }
    p->count--;
}

void play_card(GameState *g, unsigned char hand_idx, unsigned char chosen_color) {
    unsigned char cur = g->current_player;
    Player *p = &g->players[cur];
    Card c;
    unsigned char effective_color;
    unsigned char old_color = g->top_color;

    c = p->hand[hand_idx];
    clear_flags(g);
    remove_from_hand(p, hand_idx);

    effective_color = (c.color == COLOR_WILD) ? chosen_color : c.color;
    g->top_color = effective_color;
    g->top_value = c.value;
    g->top_card = c;

    if (g->discard_count < DECK_SIZE) {
        g->discard_pile[g->discard_count] = c;
        g->discard_count++;
    }

    if (p->count == 0) {
        g->flag_win = 1;
        g->winner = cur;
        return;
    }
    if (p->count == 1) {
        g->flag_uno_player = cur;
    }

    switch (c.value) {
        case VAL_SKIP:
            advance_turn(g);
            g->flag_skip = g->current_player;
            advance_turn(g);
            break;
        case VAL_REVERSE:
            g->direction = (signed char)(-g->direction);
            g->flag_reverse = cur;
            advance_turn(g);
            break;
        case VAL_DRAW2:
            advance_turn(g);
            g->flag_draw_player = g->current_player;
            g->flag_draw_count = 2;
            draw_card(g, g->current_player);
            draw_card(g, g->current_player);
            advance_turn(g);
            break;
        case VAL_WILD4: {
            unsigned char k, has_match = 0;
            for (k = 0; k < p->count; k++) {
                if (p->hand[k].color == old_color) {
                    has_match = 1;
                    break;
                }
            }
            advance_turn(g);
            g->wd4_pending = 1;
            g->wd4_player = cur;
            g->wd4_victim = g->current_player;
            g->wd4_was_legal = !has_match;
            /* Drawing/skipping happens in resolve_wd4() once the victim
               has decided whether to challenge. */
            break;
        }
        default:
            advance_turn(g);
            break;
    }
}

void resolve_wd4(GameState *g, unsigned char challenged) {
    clear_flags(g);
    if (challenged && !g->wd4_was_legal) {
        /* Challenge succeeds: the play was illegal. The original player
           draws 4 instead, and the victim's turn proceeds normally. */
        draw_card(g, g->wd4_player);
        draw_card(g, g->wd4_player);
        draw_card(g, g->wd4_player);
        draw_card(g, g->wd4_player);
        g->flag_draw_player = g->wd4_player;
        g->flag_draw_count = 4;
    } else if (challenged) {
        /* Challenge fails: the play was legal. The challenger draws 6
           (4 plus a 2-card penalty) and is skipped. */
        draw_card(g, g->wd4_victim);
        draw_card(g, g->wd4_victim);
        draw_card(g, g->wd4_victim);
        draw_card(g, g->wd4_victim);
        draw_card(g, g->wd4_victim);
        draw_card(g, g->wd4_victim);
        g->flag_draw_player = g->wd4_victim;
        g->flag_draw_count = 6;
        advance_turn(g);
    } else {
        /* No challenge: normal effect. */
        draw_card(g, g->wd4_victim);
        draw_card(g, g->wd4_victim);
        draw_card(g, g->wd4_victim);
        draw_card(g, g->wd4_victim);
        g->flag_draw_player = g->wd4_victim;
        g->flag_draw_count = 4;
        advance_turn(g);
    }
    g->wd4_pending = 0;
}
