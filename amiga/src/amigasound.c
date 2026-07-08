#include <exec/types.h>
#include <exec/memory.h>
#include <hardware/custom.h>
#include <hardware/dmabits.h>
#include <proto/exec.h>
#include "amigacon.h"
#include "amigasound.h"

/* Paula (the 4-channel PCM sound chip) has no per-channel tone generator
   the way SID/POKEY/TED do -- it plays back an arbitrary sample buffer
   on a loop, so "playing a tone" means generating a sample ourselves and
   pointing a channel at it, in CHIP RAM (the only memory the custom
   chips' DMA can reach). Channel 0 only, since these sound effects are
   never layered.

   A 16-step approximation of one sine cycle (values roughly
   64*sin(2*pi*n/16)) instead of a flat 2-level square wave -- much less
   buzzy/harsh once actually heard in the emulator. No floating point
   involved (avoids the same mathieeedoubbas.library dependency problem
   sprintf caused elsewhere in this port); this is just a fixed table. */
static const BYTE sine_wave[16] = {
    0, 24, 45, 59, 64, 59, 45, 24, 0, -24, -45, -59, -64, -59, -45, -24
};

extern volatile struct Custom custom;

#define WAVE_LEN 16 /* bytes = 8 words */
static BYTE *wave;

void snd_init(void) {
    unsigned char i;
    wave = (BYTE *)AllocMem(WAVE_LEN, MEMF_CHIP);
    for (i = 0; i < WAVE_LEN; i++) {
        wave[i] = sine_wave[i];
    }
    custom.dmacon = DMAF_SETCLR | DMAF_MASTER;
}

/* Blocking for a handful of frames (via the existing wait_vsync() frame
   pacing) rather than firing the tone and returning immediately the way
   every other port's sfx_* does -- Paula just loops the sample forever
   once DMA is enabled, so something has to decide when to turn it back
   off, and a short wait here is far simpler than threading a countdown
   through the main loop.

   Ramps volume up then back down around a flat middle stretch instead of
   snapping straight to full volume and cutting off dead, which produced
   an audible click at the start/end of every tone -- a short fade reads
   as a deliberate "blip" instead. Skipped for very short tones (under 4
   frames) since there's no room for it to read as a fade rather than
   just choppier volume steps. */
static void note(unsigned int period, unsigned char vol, unsigned char frames) {
    unsigned char i;
    custom.aud[0].ac_ptr = (UWORD *)wave;
    custom.aud[0].ac_len = WAVE_LEN / 2;
    custom.aud[0].ac_per = (UWORD)period;

    if (frames >= 4) {
        custom.aud[0].ac_vol = (UWORD)(vol / 2);
        custom.dmacon = DMAF_SETCLR | DMAF_AUD0;
        wait_vsync();
        custom.aud[0].ac_vol = vol;
        for (i = 0; i < (unsigned char)(frames - 3); i++) wait_vsync();
        custom.aud[0].ac_vol = (UWORD)(vol / 2);
        wait_vsync();
        custom.aud[0].ac_vol = 0;
        wait_vsync();
    } else {
        custom.aud[0].ac_vol = vol;
        custom.dmacon = DMAF_SETCLR | DMAF_AUD0;
        for (i = 0; i < frames; i++) wait_vsync();
    }
    custom.dmacon = DMAF_AUD0;
}

void sfx_card_play(void) {
    note(300, 40, 3);
}

void sfx_invalid(void) {
    note(900, 50, 6);
}

void sfx_draw(void) {
    note(500, 30, 3);
}

void sfx_draw_multi(unsigned char count) {
    (void)count;
    note(500, 40, 5);
}

void sfx_skip(void) {
    note(700, 40, 4);
}

void sfx_reverse(void) {
    note(400, 40, 4);
}

/* Short ascending run instead of one flat tone -- reads as a little
   "ta-da" rather than a single beep. */
void sfx_uno(void) {
    note(300, 50, 3);
    note(220, 50, 3);
    note(160, 55, 4);
}

void sfx_win(void) {
    note(400, 50, 3);
    note(300, 50, 3);
    note(220, 55, 3);
    note(150, 60, 6);
}

void sfx_challenge_success(void) {
    note(250, 50, 3);
    note(160, 55, 5);
}

/* Descending instead of ascending -- distinguishes "this went badly"
   from the success/uno chimes at a glance (well, a listen). */
void sfx_challenge_fail(void) {
    note(500, 50, 3);
    note(950, 55, 6);
}
