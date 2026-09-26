#ifndef CARDS_H
#define CARDS_H

#define COLOR_RED 0
#define COLOR_YELLOW 1
#define COLOR_GREEN 2
#define COLOR_BLUE 3
#define COLOR_WILD 4

#define VAL_SKIP 10
#define VAL_REVERSE 11
#define VAL_DRAW2 12
#define VAL_WILD 13
#define VAL_WILD4 14

#define DECK_SIZE 108

typedef struct {
    unsigned char color; /* COLOR_RED..COLOR_WILD */
    unsigned char value; /* 0-9, or VAL_* */
} Card;

void deck_build(Card deck[DECK_SIZE]);
void deck_shuffle(Card deck[], unsigned char count);
unsigned char card_is_action_or_wild(Card c);

#endif
