#ifndef PCSPK_H
#define PCSPK_H

/* The PC speaker: one square-wave voice, gated by timer channel 2 of the
   8253 PIT. Same single-voice class as the Apple II, PET and ZX Spectrum
   ports -- so chords are arpeggiated rather than played.

   Its input clock is 1.193182MHz, which is 14.31818MHz divided by 12 --
   and 14.31818 is four times the 3.579545MHz NTSC colourburst. That makes
   this the third machine in the repo whose tone constant comes off that
   same crystal, after the TI-99/4A's SN76489 and the MSX2's AY-3-8910. */

void snd_init(void);
void snd_shutdown(void);
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
