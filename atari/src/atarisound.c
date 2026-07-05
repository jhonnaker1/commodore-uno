#include <time.h>
#include "atarisound.h"

#define AUDF1 (*(unsigned char *)0xD200)
#define AUDC1 (*(unsigned char *)0xD201)
#define AUDF2 (*(unsigned char *)0xD202)
#define AUDC2 (*(unsigned char *)0xD203)
#define AUDF3 (*(unsigned char *)0xD204)
#define AUDC3 (*(unsigned char *)0xD205)
#define AUDF4 (*(unsigned char *)0xD206)
#define AUDC4 (*(unsigned char *)0xD207)
#define AUDCTL (*(unsigned char *)0xD208)

/* AUDC: bits 7-5 = distortion (5 = clean/pure tone, no noise poly),
   bit 4 = volume-only (0 = normal), bits 3-0 = volume. 0xA0 | volume
   is the standard "musical note" setting. */
#define TONE(vol) (unsigned char)(0xA0 | (vol))

static void delay_jiffies(unsigned char n) {
    clock_t target = clock() + n;
    while (clock() < target) {}
}

void snd_init(void) {
    AUDCTL = 0;
    AUDC1 = 0;
    AUDC2 = 0;
    AUDC3 = 0;
    AUDC4 = 0;
}

/* freq in Hz (approximate -- POKEY's exact divisor math depends on
   AUDCTL clock-source bits we leave at default, so like the PET port
   this is tuned by ear rather than computed precisely). */
static unsigned char freq_to_audf(unsigned int freq) {
    unsigned int f = 31000U / freq;
    if (f > 255) f = 255;
    if (f < 1) f = 1;
    return (unsigned char)f;
}

static void tone1(unsigned int freq, unsigned char vol, unsigned char jiffies) {
    AUDF1 = freq_to_audf(freq);
    AUDC1 = TONE(vol);
    delay_jiffies(jiffies);
    AUDC1 = 0;
}

static void chord2(unsigned int f1, unsigned int f2, unsigned char vol, unsigned char jiffies) {
    AUDF1 = freq_to_audf(f1);
    AUDC1 = TONE(vol);
    AUDF2 = freq_to_audf(f2);
    AUDC2 = TONE(vol);
    delay_jiffies(jiffies);
    AUDC1 = 0;
    AUDC2 = 0;
}

void sfx_card_play(void) {
    tone1(440, 10, 4);
}

void sfx_invalid(void) {
    AUDF3 = freq_to_audf(110);
    AUDC3 = TONE(10);
    delay_jiffies(12);
    AUDC3 = 0;
}

void sfx_draw(void) {
    tone1(900, 8, 3);
}

void sfx_draw_multi(unsigned char count) {
    unsigned char i;
    if (count > 6) count = 6;
    for (i = 0; i < count; i++) {
        tone1(900, 8, 3);
        delay_jiffies(3);
    }
}

void sfx_skip(void) {
    tone1(500, 10, 5);
    tone1(300, 10, 7);
}

void sfx_reverse(void) {
    chord2(400, 600, 8, 5);
    chord2(600, 400, 8, 5);
}

void sfx_uno(void) {
    tone1(440, 10, 5);
    tone1(554, 10, 5);
    tone1(659, 12, 8);
}

void sfx_win(void) {
    chord2(440, 554, 10, 6);
    chord2(554, 659, 10, 6);
    chord2(659, 880, 12, 12);
}

void sfx_challenge_success(void) {
    tone1(500, 10, 5);
    chord2(659, 880, 12, 10);
}

void sfx_challenge_fail(void) {
    tone1(300, 10, 5);
    tone1(150, 12, 12);
}
