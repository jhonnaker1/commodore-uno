#ifndef AMIGASOUND_H
#define AMIGASOUND_H

/* Paula (the Amiga's 4-channel PCM audio chip) needs sample data and DMA
   channel setup to make any sound at all -- there's no simple "set a
   tone register" the way SID/POKEY/TED have. amigasound.c generates a
   tiny square-wave sample in chip RAM at init and points channel 0 at it
   with a different period/volume/duration per effect. */

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
