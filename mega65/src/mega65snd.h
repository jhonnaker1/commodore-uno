#ifndef MEGA65SND_H
#define MEGA65SND_H

/* Sound effects on the MEGA65's SID at $D400 (C64-compatible; the MEGA65
   has two SIDs, this uses the first). Reached with plain pointer writes --
   unlike the CBM-II's bank-switched SID, the MEGA65's is directly mapped
   in C64 mode. Same sfx_* API as every other port. */

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
