#ifndef UI_H
#define UI_H

#include "game.h"

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

/* VBXE/X16 extras: positions for animations, blink cursor, win flourish */
void ui_hand_card_pos(unsigned char index, unsigned char *col, unsigned char *row);
void ui_draw_pile_pos(unsigned char *col, unsigned char *row);
void ui_top_card_pos(unsigned char *col, unsigned char *row);
void ui_opponent_pos(unsigned char idx, unsigned char *col, unsigned char *row);
unsigned char ui_suit_color(unsigned char color);
void ui_blink_cursor(unsigned char cursor, unsigned char on);
void ui_win_flourish_step(unsigned char step);

#endif
