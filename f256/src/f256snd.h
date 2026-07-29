#ifndef F256SND_H
#define F256SND_H

/* Sound effects for the Foenix F256K, driven by its onboard SID (the left
   SID sits at $D400-$D4FF, register-for-register identical to a real 6581 --
   same base address and layout as the C64, so this is a close port of the
   C64 port's sid.c). $D400 lives in the fixed I/O page 0 (MMU_IO_CTRL=0),
   the same page the Vicky video registers use, so writing it never needs
   the page-switch dance the video layer does for the $C000 char/colour
   matrices. */

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
