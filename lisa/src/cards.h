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

/* The trailing pad forces 2-byte alignment on the 68000. gcc happens to
   word-align this two-char struct already (which is why the text build has
   always worked), but making the requirement explicit matches the ST port
   and guarantees a Card is never copied by a `move.w` at an odd address --
   which would trap with an Address Error. Nothing reads _pad; Cards are
   always built field by field. */
typedef struct {
    unsigned char color; /* COLOR_RED..COLOR_WILD */
    unsigned char value; /* 0-9, or VAL_* */
    unsigned short _pad; /* alignment only */
} Card;

void deck_build(Card deck[DECK_SIZE]);
void deck_shuffle(Card deck[], unsigned char count);
unsigned char card_is_action_or_wild(Card c);

#endif
