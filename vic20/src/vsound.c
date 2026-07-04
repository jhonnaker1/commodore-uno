#include <vic20.h>
#include "vsound.h"

/* Each VIC-20 voice is a single register: bit 7 = on/off, bits 0-6 =
   frequency (higher value = higher pitch, the normal way round). Volume
   is a shared 4-bit nibble. Much simpler and better-documented than
   TED's control register, but we still always leave every voice
   explicitly off at the end of each effect as a defensive habit. */
#define GATE 0x80

static void wait_frame(void) {
    while (VIC.rasterline != 0) {}
    while (VIC.rasterline == 0) {}
}

static void hold(unsigned char frames) {
    while (frames--) wait_frame();
}

static void v1_on(unsigned char freq) {
    VIC.voice1 = (unsigned char)(GATE | (freq & 0x7F));
}
static void v1_off(void) {
    VIC.voice1 = 0;
}
static void v2_on(unsigned char freq) {
    VIC.voice2 = (unsigned char)(GATE | (freq & 0x7F));
}
static void v2_off(void) {
    VIC.voice2 = 0;
}
static void noise_on(unsigned char freq) {
    VIC.noise = (unsigned char)(GATE | (freq & 0x7F));
}
static void noise_off(void) {
    VIC.noise = 0;
}

void vsound_init(void) {
    VIC.voice1 = 0;
    VIC.voice2 = 0;
    VIC.voice3 = 0;
    VIC.noise = 0;
    VIC.volume_color = (unsigned char)((VIC.volume_color & 0xF0) | 0x0F);
}

/* Bright short blip. */
void sfx_card_play(void) {
    v1_on(100);
    hold(4);
    v1_off();
}

/* Noise burst plus a low thump. */
void sfx_draw(void) {
    noise_on(80);
    v2_on(30);
    hold(6);
    v2_off();
    noise_off();
}

/* Scales with how many cards are being forced in. */
void sfx_draw_multi(unsigned char count) {
    unsigned char n = (count > 4) ? 4 : count;
    unsigned char i;
    for (i = 0; i < n; i++) {
        noise_on((unsigned char)(70 + i * 10));
        v2_on((unsigned char)(30 + i * 8));
        hold(4);
        v2_off();
        noise_off();
        hold(2);
    }
}

/* Two low descending tones for an illegal move. */
void sfx_invalid(void) {
    v1_on(40);
    hold(8);
    v1_off();
    v1_on(25);
    hold(8);
    v1_off();
}

/* Two-note bright chime on both tone voices together. */
void sfx_uno(void) {
    unsigned char i;
    for (i = 0; i < 2; i++) {
        v1_on(110);
        v2_on(90);
        hold(6);
        v1_off();
        v2_off();
        hold(2);
    }
}

/* Quick descending sweep for a skipped turn. */
void sfx_skip(void) {
    unsigned char i;
    v1_on(120);
    for (i = 0; i < 8; i++) {
        v1_on((unsigned char)(120 - i * 10));
        wait_frame();
    }
    v1_off();
}

/* Up-then-down bounce for a direction reversal. */
void sfx_reverse(void) {
    unsigned char i;
    v1_on(40);
    for (i = 0; i < 5; i++) {
        v1_on((unsigned char)(40 + i * 12));
        wait_frame();
    }
    for (i = 0; i < 5; i++) {
        v1_on((unsigned char)(100 - i * 12));
        wait_frame();
    }
    v1_off();
}

/* Short ascending sting: the challenge succeeded. */
void sfx_challenge_success(void) {
    static const unsigned char notes[3] = {60, 90, 120};
    unsigned char i;
    for (i = 0; i < 3; i++) {
        v1_on(notes[i]);
        hold(5);
        v1_off();
    }
}

/* Single low buzz: the challenge failed. */
void sfx_challenge_fail(void) {
    v1_on(20);
    hold(12);
    v1_off();
}

/* Short arpeggio fanfare across both tone voices. */
void sfx_win(void) {
    static const unsigned char notes[6] = {50, 65, 80, 90, 100, 120};
    unsigned char i;
    v2_on(70);
    for (i = 0; i < 6; i++) {
        v1_on(notes[i]);
        hold(8);
        v1_off();
    }
    v2_off();
}
