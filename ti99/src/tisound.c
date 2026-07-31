#include <vdp.h>
#include <sound.h>
#include "tisound.h"

/* SN76489 (TI calls it the TMS9919) at >8400: three square-wave tone voices
   plus a noise voice, write-only, one byte at a time. Real tone generators,
   unlike the 1-bit beeper ports (PET/Apple/Spectrum) -- so these effects are
   proper two- and three-note figures rather than clicks, closer to the
   SID/POKEY/PSG ports.

   A tone is two bytes: a latch byte carrying the channel, the "this is a
   frequency" bit and the low 4 bits of a 10-bit divider, then a data byte
   with the high 6 bits. The chip divides its 3.579545 MHz clock by 32, so
   the divider for a given pitch is 111861/Hz. Volume is a single byte of
   *attenuation*: 0 is loudest and 15 is silent. */

/* Dividers are worked out by the macro at each call site, where the pitch is
   a literal and the compiler can fold it away. Doing the division at runtime
   would be a 32-bit divide (111861 doesn't fit in this target's 16-bit int),
   which costs a call into libgcc on a CPU with no 32-bit divide at all. */
#define SND_CLOCK 111861UL
#define HZ(hz) ((unsigned int)(SND_CLOCK / (hz)))

static void tone(unsigned char chan, unsigned int div, unsigned char atten) {
    if (div > 1023) div = 1023;
    SOUND = (unsigned char)(0x80 | (chan << 5) | (div & 0x0F));
    SOUND = (unsigned char)((div >> 4) & 0x3F);
    SOUND = (unsigned char)(0x90 | (chan << 5) | (atten & 0x0F));
}

static void silence(unsigned char chan) {
    SOUND = (unsigned char)(0x90 | (chan << 5) | 0x0F);
}

/* Blocking, like the other simple-sound ports: hold the note for the given
   number of frames, then cut it. Callers are already between redraws. */
static void hold(unsigned char frames) {
    while (frames--) vdpwaitvint();
}

static void beep(unsigned int div, unsigned char frames) {
    tone(0, div, 2);
    hold(frames);
    silence(0);
}

void snd_init(void) {
    MUTE_SOUND();
}

void sfx_card_play(void) {
    beep(HZ(800), 3);
}

void sfx_invalid(void) {
    beep(HZ(150), 9);
}

void sfx_draw(void) {
    beep(HZ(500), 3);
}

void sfx_draw_multi(unsigned char count) {
    unsigned char i;
    if (count > 4) count = 4;
    for (i = 0; i < count; i++) {
        beep(HZ(500), 3);
        hold(2);
    }
}

void sfx_skip(void) {
    beep(HZ(300), 4);
    beep(HZ(200), 4);
}

void sfx_reverse(void) {
    beep(HZ(400), 3);
    beep(HZ(300), 3);
    beep(HZ(400), 3);
}

/* UNO and the win flourish stack all three voices into a chord -- the one
   thing this chip can do that the beeper ports cannot. */
void sfx_uno(void) {
    tone(0, HZ(600), 2);
    tone(1, HZ(800), 4);
    tone(2, HZ(1000), 6);
    hold(12);
    silence(0);
    silence(1);
    silence(2);
}

void sfx_win(void) {
    beep(HZ(500), 3);
    beep(HZ(650), 3);
    beep(HZ(800), 3);
    tone(0, HZ(1000), 2);
    tone(1, HZ(800), 4);
    tone(2, HZ(500), 6);
    hold(25);
    silence(0);
    silence(1);
    silence(2);
}

void sfx_challenge_success(void) {
    beep(HZ(700), 3);
    beep(HZ(1000), 6);
}

void sfx_challenge_fail(void) {
    beep(HZ(300), 3);
    beep(HZ(150), 8);
}
