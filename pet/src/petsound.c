#include <time.h>
#include <pet.h>
#include "petsound.h"

static void delay_jiffies(unsigned char n) {
    clock_t target = clock() + n;
    while (clock() < target) {}
}

void snd_init(void) {
    VIA.ddrb |= 0x80;
    VIA.acr &= 0x3F;
}

/* freq in Hz (approx, PET's 1MHz clock / 2 / (n+2) per half-period);
   silent (acr's PB7 output disabled) when freq is 0. */
static void tone(unsigned int freq, unsigned char jiffies) {
    unsigned int n;
    if (freq == 0) {
        delay_jiffies(jiffies);
        return;
    }
    n = (unsigned int)(500000UL / freq);
    n = (n > 2) ? (unsigned int)(n - 2) : 1;
    VIA.acr = (unsigned char)((VIA.acr & 0x3F) | 0xC0);
    VIA.t1l_lo = (unsigned char)(n & 0xFF);
    VIA.t1_hi = (unsigned char)(n >> 8);
    delay_jiffies(jiffies);
    VIA.acr &= 0x3F;
}

void sfx_card_play(void) {
    tone(300, 3);
}

void sfx_invalid(void) {
    tone(110, 10);
}

void sfx_draw(void) {
    tone(700, 2);
}

void sfx_draw_multi(unsigned char count) {
    unsigned char i;
    if (count > 6) count = 6;
    for (i = 0; i < count; i++) {
        tone(700, 2);
        tone(0, 2);
    }
}

void sfx_skip(void) {
    tone(500, 4);
    tone(250, 6);
}

void sfx_reverse(void) {
    tone(400, 3);
    tone(600, 3);
    tone(400, 3);
}

void sfx_uno(void) {
    tone(440, 4);
    tone(554, 4);
    tone(659, 6);
}

void sfx_win(void) {
    tone(440, 5);
    tone(554, 5);
    tone(659, 5);
    tone(880, 10);
}

void sfx_challenge_success(void) {
    tone(500, 5);
    tone(800, 8);
}

void sfx_challenge_fail(void) {
    tone(300, 5);
    tone(150, 10);
}
