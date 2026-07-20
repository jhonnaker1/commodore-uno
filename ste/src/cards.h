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

/* The trailing pad exists purely to force 2-byte alignment on the 68000.
   With two char members the struct's alignment is 1 (vbcc's m68k backend
   sets the minimum struct alignment to 1), so a Card could land on an odd
   address -- while the compiler still copies it with a single move.w, which
   traps with an Address Error on an odd address. That bit twice: once for a
   Card array inside GameState, and again for a Card local passed by value
   into is_legal(). A member with alignment 2 makes the whole struct
   2-aligned, so every Card, wherever it lives, is safely even.
   Nothing reads _pad; Cards are always built field by field. */
typedef struct {
    unsigned char color; /* COLOR_RED..COLOR_WILD */
    unsigned char value; /* 0-9, or VAL_* */
    unsigned short _pad; /* alignment only -- see above */
} Card;

void deck_build(Card deck[DECK_SIZE]);
void deck_shuffle(Card deck[], unsigned char count);
unsigned char card_is_action_or_wild(Card c);

#endif
