#include <cbm510.h>
#include "vid510.h"
#include "snd510.h"

/* SID registers relative to $DA00 (same layout as the C64's SID at
   $D400), reached via pokebsys() -- see snd510.h. Voice 1 only, since
   these sound effects are never layered. */
#define SID_BASE 0xDA00
#define SID_V1_FREQLO (SID_BASE + 0x00)
#define SID_V1_FREQHI (SID_BASE + 0x01)
#define SID_V1_AD (SID_BASE + 0x05)
#define SID_V1_SR (SID_BASE + 0x06)
#define SID_V1_CTRL (SID_BASE + 0x04)
#define SID_VOLUME (SID_BASE + 0x18)

#define WAVE_TRIANGLE 0x10
#define WAVE_SAWTOOTH 0x20
#define WAVE_PULSE 0x40
#define GATE 0x01

void snd_init(void) {
    pokebsys(SID_VOLUME, 0x0F); /* max volume, no filter */
    pokebsys(SID_V1_AD, 0x00);  /* fast attack, no decay */
    pokebsys(SID_V1_SR, 0xF0);  /* high sustain, fast release */
}

/* Blocking for a handful of frames (via the existing wait_vsync() frame
   pacing) rather than firing the tone and returning immediately the way
   most other ports' sfx_* do -- SID keeps a voice gated on until told
   otherwise, so something has to decide when to release it, and a short
   wait here is simpler than threading a countdown through the main loop
   (same reasoning as the Amiga's Paula-based sfx_*). */
static void note(unsigned int freq, unsigned char wave, unsigned char frames) {
    pokebsys(SID_V1_FREQLO, (unsigned char)(freq & 0xFF));
    pokebsys(SID_V1_FREQHI, (unsigned char)(freq >> 8));
    pokebsys(SID_V1_CTRL, (unsigned char)(wave | GATE));
    while (frames--) wait_vsync();
    pokebsys(SID_V1_CTRL, wave); /* gate off, same waveform bits */
}

void sfx_card_play(void) {
    note(0x1800, WAVE_PULSE, 3);
}

void sfx_invalid(void) {
    note(0x0600, WAVE_SAWTOOTH, 6);
}

void sfx_draw(void) {
    note(0x1000, WAVE_TRIANGLE, 3);
}

void sfx_draw_multi(unsigned char count) {
    (void)count;
    note(0x1000, WAVE_TRIANGLE, 5);
}

void sfx_skip(void) {
    note(0x0C00, WAVE_SAWTOOTH, 4);
}

void sfx_reverse(void) {
    note(0x1400, WAVE_TRIANGLE, 4);
}

/* Short ascending run instead of one flat tone -- a little "ta-da"
   rather than a single beep, same idea as the Amiga's fanfares. */
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

/* Descending instead of ascending -- distinguishes "this went badly"
   from the success/uno chimes at a glance (well, a listen). */
void sfx_challenge_fail(void) {
    note(0x0C00, WAVE_SAWTOOTH, 3);
    note(0x0600, WAVE_SAWTOOTH, 6);
}
