#ifndef UI_VBXE_H
#define UI_VBXE_H

/* Extra declarations for the VBXE build's tile-and-animation UI, kept out
   of the shared ui.h (used by both this build and the plain atarivid.c
   build) so the plain build never needs stub implementations of these. */

/* Screen-cell positions used to aim the card-toss animation (a small
   moving colored block, redrawn cell-by-cell -- VBXE has no hardware
   sprites, unlike the C64/C128 ports this mirrors). */
void ui_hand_card_pos(unsigned char index, unsigned char *col, unsigned char *row);
void ui_draw_pile_pos(unsigned char *col, unsigned char *row);
void ui_top_card_pos(unsigned char *col, unsigned char *row);
void ui_opponent_pos(unsigned char idx, unsigned char *col, unsigned char *row);
unsigned char ui_suit_color(unsigned char color);

/* Toggles just the "[" "]" highlight cells flanking the selected hand
   tile, without touching the tile itself or the rest of the hand grid. */
void ui_blink_cursor(unsigned char cursor, unsigned char on);

/* Small rainbow flourish for the win screen (cycles a decorative strip). */
void ui_win_flourish_step(unsigned char step);

#endif
