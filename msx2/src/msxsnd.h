#ifndef MSXSND_H
#define MSXSND_H

/* AY-3-8910 (the MSX's PSG): three independent square-wave voices plus a
   noise generator. Three real voices means the UNO call and the win
   flourish are actual chords rather than the arpeggios the single-voice
   ports (PET/Apple/Spectrum) have to fake them with. */

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
