#ifndef APPLESOUND_H
#define APPLESOUND_H

/* The Apple IIe has no sound chip at all -- just a 1-bit speaker that
   toggles state on any access (read or write) to the $C030 soft switch.
   Tones are bit-banged by hitting that switch in a timed loop, the same
   technique used for the PET's VIA/CB2 speaker. One voice, no chords. */

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
