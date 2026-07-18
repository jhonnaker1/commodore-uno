#include "mega65vid.h"   /* wait_vsync() */
#include "mega65snd.h"

/* SID voice 1 at $D400 (same layout as the C64). Directly mapped in C64
   mode, so plain pointer writes work (no pokebsys/bank-switch like the
   CBM-II port needed). Voice 1 only -- these effects never layer. */
#define SID(off) (*(volatile unsigned char *)(0xD400 + (off)))
#define SID_FREQLO 0x00
#define SID_FREQHI 0x01
#define SID_CTRL 0x04
#define SID_AD 0x05
#define SID_SR 0x06
#define SID_VOLUME 0x18

#define WAVE_TRIANGLE 0x10
#define WAVE_SAWTOOTH 0x20
#define WAVE_PULSE 0x40
#define GATE 0x01

void snd_init(void) {
    SID(SID_VOLUME) = 0x0F;   /* max volume, no filter */
    SID(SID_AD) = 0x00;       /* fast attack, no decay */
    SID(SID_SR) = 0xF0;       /* high sustain, fast release */
}

/* Blocking for a few frames (paced by wait_vsync) rather than firing and
   returning -- SID keeps a voice gated until told otherwise, so a short
   wait here is simpler than threading a countdown through the main loop. */
static void note(unsigned int freq, unsigned char wave, unsigned char frames) {
    SID(SID_FREQLO) = (unsigned char)(freq & 0xFF);
    SID(SID_FREQHI) = (unsigned char)(freq >> 8);
    SID(SID_CTRL) = (unsigned char)(wave | GATE);
    while (frames--) wait_vsync();
    SID(SID_CTRL) = wave;     /* gate off, same waveform bits */
}

void sfx_card_play(void) { note(0x1800, WAVE_PULSE, 3); }
void sfx_invalid(void)   { note(0x0600, WAVE_SAWTOOTH, 6); }
void sfx_draw(void)      { note(0x1000, WAVE_TRIANGLE, 3); }

void sfx_draw_multi(unsigned char count) {
    (void)count;
    note(0x1000, WAVE_TRIANGLE, 5);
}

void sfx_skip(void)    { note(0x0C00, WAVE_SAWTOOTH, 4); }
void sfx_reverse(void) { note(0x1400, WAVE_TRIANGLE, 4); }

void sfx_uno(void) {
    note(0x1800, WAVE_PULSE, 3);
    note(0x2000, WAVE_PULSE, 3);
    note(0x2800, WAVE_PULSE, 4);
}

void sfx_win(void) {
    note(0x1000, WAVE_PULSE, 3);
    note(0x1800, WAVE_PULSE, 3);
    note(0x2000, WAVE_PULSE, 3);
    note(0x2800, WAVE_PULSE, 6);
}

void sfx_challenge_success(void) {
    note(0x1800, WAVE_TRIANGLE, 3);
    note(0x2800, WAVE_TRIANGLE, 5);
}

void sfx_challenge_fail(void) {
    note(0x0C00, WAVE_SAWTOOTH, 3);
    note(0x0600, WAVE_SAWTOOTH, 6);
}
