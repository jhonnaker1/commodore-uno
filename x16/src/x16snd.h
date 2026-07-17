#ifndef X16SND_H
#define X16SND_H

/* Sound effects on the X16's VERA PSG (Programmable Sound Generator): 16
   voices at VRAM $1F9C0, each 4 bytes (freq word, volume+L/R, waveform).
   This uses voice 0 only, since these effects never layer. Same sfx_*()
   API as every other port so the shared game loop calls it unchanged. */

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
