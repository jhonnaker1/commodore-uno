#include <cx16.h>
#include "x16vera.h"   /* wait_vsync() */
#include "x16snd.h"

/* VERA PSG voice 0 lives at VRAM $1F9C0:
     byte0 = frequency word (7:0)
     byte1 = frequency word (15:8)   output_hz = freq_word * 25MHz/512/2^17
     byte2 = Right(b7) Left(b6) Volume(0-63)
     byte3 = Waveform(b7:6: 0=pulse 1=saw 2=tri 3=noise) PulseWidth(0-63)
   Frequency word ~= Hz * 2.684. */
#define PSG_VOICE0 0x1F9C0UL

#define WAVE_PULSE 0
#define WAVE_SAW   1
#define WAVE_TRI   2

/* Convenient frequency words (see formula above). */
#define F_LOW  1000   /* ~372 Hz */
#define F_MID  1600   /* ~596 Hz */
#define F_HIGH 2200   /* ~819 Hz */
#define F_TOP  2800   /* ~1043 Hz */

static void psg_seek(unsigned char byte) {
    VERA.control = 0;
    VERA.address = (unsigned int)((PSG_VOICE0 + byte) & 0xFFFF);
    VERA.address_hi = (unsigned char)(((PSG_VOICE0 >> 16) & 1) | (1 << 4));
}

/* Plays freq_word on the given waveform for `frames` vsyncs, then silences
   the voice. Blocking (paced by wait_vsync) like the SID/Paula ports --
   simpler than threading a release countdown through the main loop. */
static void note(unsigned int fw, unsigned char wave, unsigned char frames) {
    psg_seek(0);
    VERA.data0 = (unsigned char)(fw & 0xFF);
    VERA.data0 = (unsigned char)(fw >> 8);
    VERA.data0 = (unsigned char)(0xC0 | 48);          /* L+R, volume 48/63 */
    VERA.data0 = (unsigned char)((wave << 6) | 0x3F); /* 50% pulse / full-scale XOR */
    while (frames--) wait_vsync();
    psg_seek(2);
    VERA.data0 = 0x00;                                /* volume 0 = silent */
}

void snd_init(void) {
    psg_seek(2);
    VERA.data0 = 0x00;   /* voice 0 silent to start */
}

void sfx_card_play(void) { note(F_MID, WAVE_PULSE, 3); }
void sfx_invalid(void)   { note(F_LOW, WAVE_SAW, 6); }
void sfx_draw(void)      { note(F_MID, WAVE_TRI, 3); }

void sfx_draw_multi(unsigned char count) {
    (void)count;
    note(F_MID, WAVE_TRI, 5);
}

void sfx_skip(void)    { note(F_LOW + 400, WAVE_SAW, 4); }
void sfx_reverse(void) { note(F_MID - 200, WAVE_TRI, 4); }

/* Short ascending run -- a little "ta-da" rather than a single beep. */
void sfx_uno(void) {
    note(F_MID, WAVE_PULSE, 3);
    note(F_HIGH, WAVE_PULSE, 3);
    note(F_TOP, WAVE_PULSE, 4);
}

void sfx_win(void) {
    note(F_LOW, WAVE_PULSE, 3);
    note(F_MID, WAVE_PULSE, 3);
    note(F_HIGH, WAVE_PULSE, 3);
    note(F_TOP, WAVE_PULSE, 6);
}

void sfx_challenge_success(void) {
    note(F_MID, WAVE_TRI, 3);
    note(F_TOP, WAVE_TRI, 5);
}

/* Descending -- distinguishes "this went badly" from the success/uno chimes. */
void sfx_challenge_fail(void) {
    note(F_MID, WAVE_SAW, 3);
    note(F_LOW, WAVE_SAW, 6);
}
