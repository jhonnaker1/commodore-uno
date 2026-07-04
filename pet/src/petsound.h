#ifndef PETSOUND_H
#define PETSOUND_H

/* The 4032 has no sound chip -- just a single-voice piezo speaker driven
   off the onboard 6522 VIA's CB2 pin (confirmed via VICE's own -cb2lowpass
   option, which documents CB2 as the PET's speaker line; an earlier
   attempt targeted PB7 instead, which produced no sound at all). CB2 has
   no hardware auto-toggle mode the way PB7 does, so tones are bit-banged:
   the CPU manually flips CB2 high/low in a timed loop via the VIA's PCR
   manual-output-level bits. One voice, no ADSR/filter, so effects are
   short tone bursts / simple sequences rather than anything SID/TED-like. */

void snd_init(void);
void sfx_card_play(void);
void sfx_invalid(void);
void sfx_draw(void);
void sfx_draw_multi(unsigned char count);
void sfx_skip(void);
void sfx_reverse(void);
void sfx_uno(void);
void sfx_win(void);
void sfx_challenge_success(void);
void sfx_challenge_fail(void);

#endif
