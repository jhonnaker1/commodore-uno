#ifndef STESOUND_H
#define STESOUND_H

/* YM2149 sound for the ST/STE, driven through XBIOS Dosound(). Dosound
   plays a command sequence asynchronously off the 50Hz VBL interrupt, so
   effects never block the game loop -- and it needs no supervisor mode.
   (The STE's DMA sound hardware could play samples instead, but the PSG
   keeps the port running identically on a plain ST.) */

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
