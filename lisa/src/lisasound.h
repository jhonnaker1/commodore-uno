#ifndef LISASOUND_H
#define LISASOUND_H

/* Sound effects for the Lisa, on its speaker (lisasound.c). Same set as
   every port's sound header, plus snd_tick(), which wait_vsync() calls
   once a frame to play the queued notes. */

void snd_init(void);
void snd_tick(void);
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
