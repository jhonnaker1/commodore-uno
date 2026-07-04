#include "petsound.h"

#define PCR (*(unsigned char *)0xE84C)

void snd_init(void) {
    PCR = (PCR & 0x1F) | 0xC0; /* CB2 manual output, low (silent) */
}

/* Bit-bangs a square wave on CB2: delay controls pitch (smaller = higher),
   cycles controls how long the tone plays. There's no way to derive exact
   Hz from cc65's compiled loop timing without hardware in hand, so these
   are tuned by ear against the emulator rather than computed. */
static void tone(unsigned int delay, unsigned int cycles) {
    unsigned int i, j;
    for (j = 0; j < cycles; j++) {
        PCR = (PCR & 0x1F) | 0xE0;
        for (i = 0; i < delay; i++) { }
        PCR = (PCR & 0x1F) | 0xC0;
        for (i = 0; i < delay; i++) { }
    }
}

static void silence(unsigned int cycles) {
    unsigned int i, j;
    for (j = 0; j < cycles; j++) {
        for (i = 0; i < 40; i++) { }
    }
}

void sfx_card_play(void) {
    tone(40, 40);
}

void sfx_invalid(void) {
    tone(150, 50);
}

void sfx_draw(void) {
    tone(20, 25);
}

void sfx_draw_multi(unsigned char count) {
    unsigned char i;
    if (count > 4) count = 4;
    for (i = 0; i < count; i++) {
        tone(20, 25);
        silence(15);
    }
}

void sfx_skip(void) {
    tone(60, 30);
    tone(100, 35);
}

void sfx_reverse(void) {
    tone(70, 25);
    tone(45, 25);
    tone(70, 25);
}

void sfx_uno(void) {
    tone(90, 25);
    tone(65, 25);
    tone(45, 30);
}

void sfx_win(void) {
    tone(90, 25);
    tone(65, 25);
    tone(45, 25);
    tone(25, 40);
}

void sfx_challenge_success(void) {
    tone(60, 25);
    tone(25, 35);
}

void sfx_challenge_fail(void) {
    tone(45, 25);
    tone(150, 40);
}
