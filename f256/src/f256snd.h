#ifndef F256SND_H
#define F256SND_H

/* Sound-effect interface, shared shape with the other ports. The F256 has
   dual SID; these are stubs for now (silent) so the game is playable, with
   real SID effects a planned follow-up. */

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
