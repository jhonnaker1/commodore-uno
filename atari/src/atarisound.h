#ifndef ATARISOUND_H
#define ATARISOUND_H

/* POKEY has 4 independent tone channels (much richer than the single-
   voice PET beeper, though without SID's filter/ADSR). Driven with
   direct register writes at $D200-$D208, since cc65 doesn't expose a
   POKEY abstraction the way conio.h abstracts the screen. */

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
