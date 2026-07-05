#include "applesound.h"

#define SPKR (*(unsigned char *)0xC030)

/* Toggles the speaker in a timed loop: delay controls pitch (smaller =
   higher), cycles controls how long the tone plays. Like the PET's CB2
   beeper, there's no way to derive exact Hz from cc65's compiled loop
   timing without hardware in hand, so these are tuned by ear. */
static void tone(unsigned int delay, unsigned int cycles) {
    unsigned int i, j;
    for (j = 0; j < cycles; j++) {
        (void)SPKR;
        for (i = 0; i < delay; i++) { }
    }
}

static void silence(unsigned int cycles) {
    unsigned int i, j;
    for (j = 0; j < cycles; j++) {
        for (i = 0; i < 40; i++) { }
    }
}

void snd_init(void) {
}

void sfx_card_play(void) {
    tone(30, 60);
}

void sfx_invalid(void) {
    tone(120, 50);
}

void sfx_draw(void) {
    tone(15, 40);
}

void sfx_draw_multi(unsigned char count) {
    unsigned char i;
    if (count > 4) count = 4;
    for (i = 0; i < count; i++) {
        tone(15, 40);
        silence(15);
    }
}

void sfx_skip(void) {
    tone(45, 60);
    tone(80, 70);
}

void sfx_reverse(void) {
    tone(55, 50);
    tone(35, 50);
    tone(55, 50);
}

void sfx_uno(void) {
    tone(70, 50);
    tone(50, 50);
    tone(35, 70);
}

void sfx_win(void) {
    tone(70, 50);
    tone(50, 50);
    tone(35, 50);
    tone(20, 100);
}

void sfx_challenge_success(void) {
    tone(45, 50);
    tone(20, 80);
}

void sfx_challenge_fail(void) {
    tone(35, 50);
    tone(120, 80);
}
