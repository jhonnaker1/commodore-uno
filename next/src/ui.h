#ifndef UI_H
#define UI_H

#include "game.h"

/* Two rows of ten: 512x342 is room for twenty whole cards, so unlike the
   Amiga/ST fans nothing overlaps. Past twenty, the rows scroll with the
   cursor (see ui_draw_hand). */
#define HAND_VISIBLE 20

void ui_title_screen(void);
void ui_draw_frame(void);
void ui_draw_opponents(GameState *g);
void ui_draw_table(GameState *g);
void ui_draw_hand(GameState *g, unsigned char cursor);
void ui_message(const char *line1, const char *line2);
void ui_draw_color_picker(unsigned char selected);
void ui_clear_color_picker(void);
void ui_game_over_screen(unsigned char human_won, unsigned char winner_idx);
void ui_draw_challenge_prompt(unsigned char victim, unsigned char player, unsigned char selected_yes);
void ui_event_challenge_result(unsigned char victim, unsigned char player, unsigned char succeeded);

void ui_event_skip(unsigned char idx);
void ui_event_reverse(unsigned char idx);
void ui_event_draw(unsigned char idx, unsigned char count);
void ui_event_uno(unsigned char idx);
void ui_event_invalid(void);
void ui_event_drew_one(unsigned char idx);
void ui_event_thinking(unsigned char idx);

/* ---- mouse hit-testing (the Mac and NeXT ports) ----
   The UI knows what it last drew, so it answers "what did this click hit?"
   for input.c. Which targets are live depends on the screen showing. */
#define UI_MODE_PRESS 0     /* title / game over: any click continues */
#define UI_MODE_GAME 1      /* hand cards and the draw pile */
#define UI_MODE_PICKER 2    /* the four colour choices */
#define UI_MODE_CHALLENGE 3 /* Yes / No */

#define HIT_NONE 0xFF
#define HIT_FIRE 0xFE
#define HIT_DRAW 0xFD
/* anything else: a hand index (GAME), a colour 0-3 (PICKER), or
   1 = Yes / 0 = No (CHALLENGE) */

unsigned char ui_mode(void);
unsigned char ui_hit(short x, short y);

#endif
