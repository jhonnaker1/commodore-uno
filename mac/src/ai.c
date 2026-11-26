#include "ai.h"

unsigned char ai_choose_card(GameState *g, unsigned char player_idx) {
    Player *p = &g->players[player_idx];
    unsigned char i;
    unsigned char wild_idx = NONE;

    for (i = 0; i < p->count; i++) {
        if (!is_legal(g, p->hand[i])) continue;
        if (p->hand[i].color == COLOR_WILD) {
            if (wild_idx == NONE) wild_idx = i;
        } else {
            return i; /* prefer a non-wild legal card, keep wilds in reserve */
        }
    }
    return wild_idx;
}

unsigned char ai_choose_color(GameState *g, unsigned char player_idx) {
    Player *p = &g->players[player_idx];
    unsigned char counts[4] = {0, 0, 0, 0};
    unsigned char i, best = COLOR_RED;

    for (i = 0; i < p->count; i++) {
        if (p->hand[i].color <= COLOR_BLUE) {
            counts[p->hand[i].color]++;
        }
    }
    for (i = 1; i <= COLOR_BLUE; i++) {
        if (counts[i] > counts[best]) best = i;
    }
    return best;
}

unsigned char ai_should_challenge_wd4(GameState *g, unsigned char player_idx) {
    /* The AI can't see whether the play was actually legal (that would be
       cheating), so it just weighs the gamble: a failed challenge costs 6
       cards instead of 4, worth risking mainly when already low on cards. */
    return (g->players[player_idx].count <= 4) ? 1 : 0;
}
