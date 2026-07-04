#include "sid.h"

#define SID_V1_FREQ_LO (*(unsigned char *)0xD400)
#define SID_V1_FREQ_HI (*(unsigned char *)0xD401)
#define SID_V1_PW_LO (*(unsigned char *)0xD402)
#define SID_V1_PW_HI (*(unsigned char *)0xD403)
#define SID_V1_CTRL (*(unsigned char *)0xD404)
#define SID_V1_AD (*(unsigned char *)0xD405)
#define SID_V1_SR (*(unsigned char *)0xD406)
#define SID_VOLUME (*(unsigned char *)0xD418)
#define VIC_RASTER (*(unsigned char *)0xD012)

#define WAVE_TRIANGLE 0x10
#define WAVE_SAWTOOTH 0x20
#define WAVE_PULSE 0x40
#define WAVE_NOISE 0x80
#define GATE 0x01

/* Approximate 16-bit SID frequency register values for C4..C5 (PAL clock). */
static const unsigned int NOTE_FREQ[8] = {
    4459, 5002, 5615, 5949, 6678, 7495, 8412, 8917
};

static void wait_frame(void) {
    while (VIC_RASTER != 0) {}
    while (VIC_RASTER == 0) {}
}

void sid_init(void) {
    unsigned char i;
    for (i = 0; i < 25; i++) {
        *((unsigned char *)0xD400 + i) = 0;
    }
    SID_VOLUME = 0x0F;
    SID_V1_PW_LO = 0x00;
    SID_V1_PW_HI = 0x08; /* ~50% duty pulse */
}

static void note_on(unsigned int freq, unsigned char wave, unsigned char decay) {
    SID_V1_FREQ_LO = (unsigned char)(freq & 0xFF);
    SID_V1_FREQ_HI = (unsigned char)(freq >> 8);
    SID_V1_AD = decay;
    SID_V1_SR = 0x00;
    SID_V1_CTRL = wave | GATE;
}

static void note_off(unsigned char wave) {
    SID_V1_CTRL = wave;
}

static void hold(unsigned char frames) {
    while (frames--) wait_frame();
}

void sfx_card_play(void) {
    note_on(8917, WAVE_PULSE, 0x08);
    hold(4);
    note_off(WAVE_PULSE);
}

void sfx_draw(void) {
    note_on(3000, WAVE_NOISE, 0x06);
    hold(5);
    note_off(WAVE_NOISE);
}

void sfx_invalid(void) {
    note_on(1200, WAVE_SAWTOOTH, 0x04);
    hold(8);
    note_off(WAVE_SAWTOOTH);
    note_on(900, WAVE_SAWTOOTH, 0x04);
    hold(8);
    note_off(WAVE_SAWTOOTH);
}

void sfx_uno(void) {
    note_on(NOTE_FREQ[7], WAVE_PULSE, 0x08);
    hold(6);
    note_off(WAVE_PULSE);
    note_on(NOTE_FREQ[7], WAVE_PULSE, 0x08);
    hold(6);
    note_off(WAVE_PULSE);
}

void sfx_turn(void) {
    note_on(NOTE_FREQ[4], WAVE_TRIANGLE, 0x06);
    hold(3);
    note_off(WAVE_TRIANGLE);
}

void sfx_win(void) {
    unsigned char i;
    static const unsigned char tune[6] = {0, 2, 4, 5, 6, 7};
    for (i = 0; i < 6; i++) {
        note_on(NOTE_FREQ[tune[i]], WAVE_PULSE, 0x09);
        hold(8);
        note_off(WAVE_PULSE);
    }
}
