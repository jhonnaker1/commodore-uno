#include <stdlib.h>

/* A 16-bit xorshift rather than the usual LCG: the classic
   seed*1103515245+12345 needs a 32-bit multiply, and this backend has no
   hardware for one -- it would drag in a libgcc helper and run slowly on a
   3 MHz 16-bit CPU. The (7,9,8) triple is a full-period xorshift over 16-bit
   state, which is plenty for shuffling a 108-card deck. */

static unsigned int state = 1;

void srand(unsigned int seed) {
    state = seed ? seed : 1;
}

int rand(void) {
    unsigned int x = state;
    x ^= (unsigned int)(x << 7);
    x ^= (unsigned int)(x >> 9);
    x ^= (unsigned int)(x << 8);
    state = x;
    return (int)(x & 0x7FFF);
}
