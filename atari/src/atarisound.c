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

/* With AUDCTL = 0 every channel runs from POKEY's base clock as a plain
   8-bit divider, and the hardware adds one to AUDF:

       f_out = base / (2 * (AUDF + 1))    ->    AUDF = base/(2*f) - 1

   base is 63921 Hz on NTSC (63337 on PAL), so the halved constant below
   is all the per-note math needs. Rounding to nearest before subtracting
   matters: plain truncation lands A440 on AUDF 71 (444 Hz) where the
   correct value is 72 ($48) at 437.8 Hz.

   An earlier version approximated this as 31000/freq with no -1, tuned
   by ear; that ran about 39 cents sharp (A440 came out at 450 Hz).

   Floor: with an 8-bit divider on the 64 kHz base the lowest reachable
   tone is 63921/512 = ~125 Hz, so sfx_invalid()'s nominal 110 Hz clamps
   there. Going lower needs AUDCTL bit 0 (15 kHz base), which would drop
   every channel four octaves, so it isn't worth it for one effect. */
#define POKEY_HALF_BASE 31960U  /* NTSC 63921/2; PAL would be 31668 */

static unsigned char freq_to_audf(unsigned int freq) {
    unsigned int n = (POKEY_HALF_BASE + (freq >> 1)) / freq; /* round */
    if (n == 0) return 0;
    n--;
    if (n > 255) n = 255;
    return (unsigned char)n;
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
