#ifndef PETSOUND_H
#define PETSOUND_H

/* The 4032 has no sound chip -- just a single-voice piezo speaker driven
   off the onboard 6522 VIA's PB7 pin (free-running square wave off Timer
   1). One voice, no ADSR/filter, so effects are short tone bursts / simple
   sequences rather than anything SID/TED-like. */

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
