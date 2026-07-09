#ifndef SND510_H
#define SND510_H

/* Real SID chip here (same as the C64's), reached via pokebsys() since
   it lives in the same bank-switched "system bank" as video/color RAM
   (see vid510.h) that plain pointer writes can't reach -- confirmed
   working empirically (an audible tone via pokebsys() writes to $DA00+
   registers), unlike the video-RAM address guess that never panned out. */

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
