#ifndef GAME_H
#define GAME_H

#include "cards.h"

#define NUM_PLAYERS 4
#define MAX_HAND 40
#define NONE 0xFF

typedef struct {
    Card hand[MAX_HAND];
    unsigned char count;
    unsigned char is_human;
} Player;

/* Field order matters on a 68000. Card is two bytes with only byte
   alignment, so a Card array placed after an odd number of scalar bytes
   starts on an odd address -- and the compiler renders a two-byte Card
   assignment as a single move.w, which traps with an Address Error when
   that address is odd. (The original layout put draw_count between the two
   piles, landing discard_pile on offset 217; harmless on the 6502 ports,
   fatal on the ST/Amiga.) Keeping every Card-holding member at the front,
   ahead of the scalars, keeps them all on even offsets. */
typedef struct {
    Card draw_pile[DECK_SIZE];
    Card discard_pile[DECK_SIZE];
    Card top_card;
    Player players[NUM_PLAYERS];

    unsigned char draw_count;
    unsigned char discard_count;
    unsigned char current_player;
    signed char direction;

    unsigned char top_color;
    unsigned char top_value;

    /* Event flags set by the most recent action, consumed by the caller. */
    unsigned char flag_win;
    unsigned char winner;
    unsigned char flag_uno_player; /* NONE or player index who just hit 1 card */
    unsigned char flag_skip;       /* player index skipped, or NONE */
    unsigned char flag_reverse;
    unsigned char flag_draw_player; /* player index forced to draw, or NONE */
    unsigned char flag_draw_count;

    /* A Wild Draw Four was just played and awaits the victim's challenge
       decision before its effect (draw + skip) is actually applied. */
    unsigned char wd4_pending;
    unsigned char wd4_player;    /* who played the wild draw four */
    unsigned char wd4_victim;    /* next player, who may challenge it */
    unsigned char wd4_was_legal; /* 1 if the player had no card of the old color */
} GameState;

void game_new(GameState *g);
unsigned char is_legal(GameState *g, Card c);
Card draw_card(GameState *g, unsigned char player_idx);
void play_card(GameState *g, unsigned char hand_idx, unsigned char chosen_color);
void clear_flags(GameState *g);
void end_turn_no_play(GameState *g);
void resolve_wd4(GameState *g, unsigned char challenged);

#endif
