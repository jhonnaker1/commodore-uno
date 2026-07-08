#ifndef UI_H
#define UI_H

#include "game.h"

/* 9 per row x 2 rows (see ui.c's HAND_PER_ROW) -- not 20 the way most
   other ports allow, since the Amiga's actually-visible screen width is
   narrower than assumed and 9/row is what safely fits. */
#define HAND_VISIBLE 18

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

#endif
