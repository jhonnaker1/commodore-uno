#include "sid.h"

#define SID_V1_FREQ_LO (*(unsigned char *)0xD400)
#define SID_V1_FREQ_HI (*(unsigned char *)0xD401)
#define SID_V1_PW_LO (*(unsigned char *)0xD402)
#define SID_V1_PW_HI (*(unsigned char *)0xD403)
#define SID_V1_CTRL (*(unsigned char *)0xD404)
#define SID_V1_AD (*(unsigned char *)0xD405)
#define SID_V1_SR (*(unsigned char *)0xD406)

#define SID_V2_FREQ_LO (*(unsigned char *)0xD407)
#define SID_V2_FREQ_HI (*(unsigned char *)0xD408)
#define SID_V2_PW_LO (*(unsigned char *)0xD409)
#define SID_V2_PW_HI (*(unsigned char *)0xD40A)
#define SID_V2_CTRL (*(unsigned char *)0xD40B)
#define SID_V2_AD (*(unsigned char *)0xD40C)
#define SID_V2_SR (*(unsigned char *)0xD40D)

#define SID_V3_FREQ_LO (*(unsigned char *)0xD40E)
#define SID_V3_FREQ_HI (*(unsigned char *)0xD40F)
#define SID_V3_PW_LO (*(unsigned char *)0xD410)
#define SID_V3_PW_HI (*(unsigned char *)0xD411)
#define SID_V3_CTRL (*(unsigned char *)0xD412)
#define SID_V3_AD (*(unsigned char *)0xD413)
#define SID_V3_SR (*(unsigned char *)0xD414)

#define SID_FC_LO (*(unsigned char *)0xD415)
#define SID_FC_HI (*(unsigned char *)0xD416)
#define SID_RES_FILT (*(unsigned char *)0xD417)
#define SID_MODE_VOL (*(unsigned char *)0xD418)
#define VIC_RASTER (*(unsigned char *)0xD012)

#define WAVE_TRIANGLE 0x10
#define WAVE_SAWTOOTH 0x20
#define WAVE_PULSE 0x40
#define WAVE_NOISE 0x80
#define RING_MOD 0x04
#define GATE 0x01

/* Approximate 16-bit SID frequency register values for C4..C5 (PAL clock). */
static const unsigned int NOTE_FREQ[8] = {
    4459, 5002, 5615, 5949, 6678, 7495, 8412, 8917
};

static void wait_frame(void) {
    while (VIC_RASTER != 0) {}
    while (VIC_RASTER == 0) {}
}

static void hold(unsigned char frames) {
    while (frames--) wait_frame();
}

void sid_init(void) {
    unsigned char i;
    for (i = 0; i < 25; i++) {
        *((unsigned char *)0xD400 + i) = 0;
    }
    SID_MODE_VOL = 0x0F; /* filter off, voice 3 audible, full volume */
    SID_RES_FILT = 0x00;
    SID_V1_PW_LO = 0x00;
    SID_V1_PW_HI = 0x08; /* ~50% duty pulse */
    SID_V2_PW_LO = 0x00;
    SID_V2_PW_HI = 0x08;
    SID_V3_PW_LO = 0x00;
    SID_V3_PW_HI = 0x08;
}

/* --- Voice 1 --- */
static void set_freq1(unsigned int freq) {
    SID_V1_FREQ_LO = (unsigned char)(freq & 0xFF);
    SID_V1_FREQ_HI = (unsigned char)(freq >> 8);
}
static void note1_on(unsigned int freq, unsigned char wave, unsigned char decay) {
    set_freq1(freq);
    SID_V1_AD = decay;
    SID_V1_SR = 0x00;
    SID_V1_CTRL = wave | GATE;
}
static void note1_off(unsigned char wave) {
    SID_V1_CTRL = wave;
}

/* --- Voice 2 --- */
static void set_freq2(unsigned int freq) {
    SID_V2_FREQ_LO = (unsigned char)(freq & 0xFF);
    SID_V2_FREQ_HI = (unsigned char)(freq >> 8);
}
static void note2_on(unsigned int freq, unsigned char wave, unsigned char decay) {
    set_freq2(freq);
    SID_V2_AD = decay;
    SID_V2_SR = 0x00;
    SID_V2_CTRL = wave | GATE;
}
static void note2_off(unsigned char wave) {
    SID_V2_CTRL = wave;
}

/* --- Voice 3 --- */
static void set_freq3(unsigned int freq) {
    SID_V3_FREQ_LO = (unsigned char)(freq & 0xFF);
    SID_V3_FREQ_HI = (unsigned char)(freq >> 8);
}
static void note3_on(unsigned int freq, unsigned char wave, unsigned char decay) {
    set_freq3(freq);
    SID_V3_AD = decay;
    SID_V3_SR = 0x00;
    SID_V3_CTRL = wave | GATE;
}
static void note3_off(unsigned char wave) {
    SID_V3_CTRL = wave;
}

static void set_filter_cutoff(unsigned int cutoff11) {
    SID_FC_LO = (unsigned char)(cutoff11 & 0x07);
    SID_FC_HI = (unsigned char)(cutoff11 >> 3);
}

void sfx_card_play(void) {
    unsigned char i;
    SID_RES_FILT = 0x81;  /* resonance 8, route voice 1 through the filter */
    SID_MODE_VOL = 0x1F;  /* lowpass on, volume 15 */
    note1_on(8917, WAVE_PULSE, 0x08);
    for (i = 0; i < 4; i++) {
        set_filter_cutoff((unsigned int)(300 + i * 450));
        wait_frame();
    }
    note1_off(WAVE_PULSE);
    SID_RES_FILT = 0x00;
    SID_MODE_VOL = 0x0F;
}

void sfx_draw(void) {
    note1_on(3000, WAVE_NOISE, 0x06);
    note2_on(1800, WAVE_TRIANGLE, 0x05);
    hold(6);
    note1_off(WAVE_NOISE);
    note2_off(WAVE_TRIANGLE);
}

/* Scales with how many cards are being forced in: a rattling volley of
   noise+thump hits instead of one, up to 4 audible hits. */
void sfx_draw_multi(unsigned char count) {
    unsigned char n = (count > 4) ? 4 : count;
    unsigned char i;
    for (i = 0; i < n; i++) {
        note1_on((unsigned int)(2600 - i * 200), WAVE_NOISE, 0x05);
        note2_on((unsigned int)(1600 - i * 100), WAVE_TRIANGLE, 0x04);
        hold(4);
        note1_off(WAVE_NOISE);
        note2_off(WAVE_TRIANGLE);
        hold(2);
    }
}

/* Ring-modulated triangle (voice 1, modulated by voice 3's oscillator) makes
   a harsh, metallic "no" buzz -- voice 3 itself is disconnected from the
   mix via the MODE_VOL bit7, so only the ring-mod effect is heard. */
void sfx_invalid(void) {
    SID_MODE_VOL = 0x8F;
    set_freq3(500);
    SID_V3_CTRL = WAVE_TRIANGLE | GATE;

    note1_on(1100, (unsigned char)(WAVE_TRIANGLE | RING_MOD), 0x05);
    hold(8);
    note1_off((unsigned char)(WAVE_TRIANGLE | RING_MOD));
    note1_on(800, (unsigned char)(WAVE_TRIANGLE | RING_MOD), 0x05);
    hold(8);
    note1_off((unsigned char)(WAVE_TRIANGLE | RING_MOD));

    SID_V3_CTRL = 0;
    SID_MODE_VOL = 0x0F;
}

/* Bright two-voice chime a fifth apart, with a filter shimmer. */
void sfx_uno(void) {
    unsigned char i;
    SID_RES_FILT = 0x83;
    SID_MODE_VOL = 0x1F;
    for (i = 0; i < 2; i++) {
        note1_on(NOTE_FREQ[7], WAVE_PULSE, 0x09);
        note2_on(NOTE_FREQ[4], WAVE_PULSE, 0x09);
        set_filter_cutoff(1800);
        hold(6);
        note1_off(WAVE_PULSE);
        note2_off(WAVE_PULSE);
        set_filter_cutoff(600);
        hold(2);
    }
    SID_RES_FILT = 0x00;
    SID_MODE_VOL = 0x0F;
}

/* Quick descending sawtooth "whoosh" for a skipped turn. */
void sfx_skip(void) {
    unsigned char i;
    set_freq1(6000);
    SID_V1_AD = 0x04;
    SID_V1_SR = 0x00;
    SID_V1_CTRL = WAVE_SAWTOOTH | GATE;
    for (i = 0; i < 8; i++) {
        set_freq1((unsigned int)(6000 - i * 650));
        wait_frame();
    }
    note1_off(WAVE_SAWTOOTH);
}

/* An up-then-down triangle bounce for a direction reversal. */
void sfx_reverse(void) {
    unsigned char i;
    set_freq1(3000);
    SID_V1_AD = 0x03;
    SID_V1_SR = 0x00;
    SID_V1_CTRL = WAVE_TRIANGLE | GATE;
    for (i = 0; i < 5; i++) {
        set_freq1((unsigned int)(3000 + i * 500));
        wait_frame();
    }
    for (i = 0; i < 5; i++) {
        set_freq1((unsigned int)(5500 - i * 500));
        wait_frame();
    }
    note1_off(WAVE_TRIANGLE);
}

/* A short ascending sting: the challenged play turned out to be illegal. */
void sfx_challenge_success(void) {
    unsigned char i;
    static const unsigned char notes[3] = {2, 5, 7};
    for (i = 0; i < 3; i++) {
        note1_on(NOTE_FREQ[notes[i]], WAVE_PULSE, 0x07);
        hold(5);
        note1_off(WAVE_PULSE);
    }
}

/* A single low buzz: the challenge was wrong. */
void sfx_challenge_fail(void) {
    note1_on(700, WAVE_SAWTOOTH, 0x05);
    hold(12);
    note1_off(WAVE_SAWTOOTH);
}

/* Opening chord (three voices through a swept filter) then the arpeggio. */
void sfx_win(void) {
    unsigned char i;
    static const unsigned char tune[6] = {0, 2, 4, 5, 6, 7};

    SID_RES_FILT = 0x8F;
    SID_MODE_VOL = 0x1F;
    note1_on(NOTE_FREQ[0], WAVE_PULSE, 0x0A);
    note2_on(NOTE_FREQ[2], WAVE_PULSE, 0x0A);
    note3_on(NOTE_FREQ[4], WAVE_PULSE, 0x0A);
    for (i = 0; i < 12; i++) {
        set_filter_cutoff((unsigned int)(200 + i * 160));
        wait_frame();
    }
    note1_off(WAVE_PULSE);
    note2_off(WAVE_PULSE);
    note3_off(WAVE_PULSE);
    SID_RES_FILT = 0x00;
    SID_MODE_VOL = 0x0F;

    for (i = 0; i < 6; i++) {
        note1_on(NOTE_FREQ[tune[i]], WAVE_PULSE, 0x09);
        hold(8);
        note1_off(WAVE_PULSE);
    }
}
