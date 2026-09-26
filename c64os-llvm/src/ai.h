#ifndef AI_H
#define AI_H

#include "game.h"

unsigned char ai_choose_card(GameState *g, unsigned char player_idx);
unsigned char ai_choose_color(GameState *g, unsigned char player_idx);
unsigned char ai_should_challenge_wd4(GameState *g, unsigned char player_idx);

#endif
