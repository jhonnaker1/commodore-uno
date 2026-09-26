#include <stdlib.h>
#include "cards.h"

void deck_build(Card deck[DECK_SIZE]) {
    unsigned char idx = 0;
    unsigned char color, val, n;

    for (color = 0; color <= COLOR_BLUE; color++) {
        deck[idx].color = color;
        deck[idx].value = 0;
        idx++;
        for (val = 1; val <= 9; val++) {
            for (n = 0; n < 2; n++) {
                deck[idx].color = color;
                deck[idx].value = val;
                idx++;
            }
        }
        for (val = VAL_SKIP; val <= VAL_DRAW2; val++) {
            for (n = 0; n < 2; n++) {
                deck[idx].color = color;
                deck[idx].value = val;
                idx++;
            }
        }
    }
    for (n = 0; n < 4; n++) {
        deck[idx].color = COLOR_WILD;
        deck[idx].value = VAL_WILD;
        idx++;
    }
    for (n = 0; n < 4; n++) {
        deck[idx].color = COLOR_WILD;
        deck[idx].value = VAL_WILD4;
        idx++;
    }
}

void deck_shuffle(Card deck[], unsigned char count) {
    unsigned char i, j;
    Card tmp;
    for (i = count - 1; i > 0; i--) {
        j = rand() % (i + 1);
        tmp = deck[i];
        deck[i] = deck[j];
        deck[j] = tmp;
    }
}

unsigned char card_is_action_or_wild(Card c) {
    return c.value >= VAL_SKIP;
}
