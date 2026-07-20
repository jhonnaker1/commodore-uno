#include <tos.h>
#include "stesound.h"

/* Dosound command format:
     $00-$0F <val>   write <val> to that YM2149 register
     $80 <val>       load the temporary register
     $81 <reg> <step> <end>   slide the temp register into <reg>
     $82-$FF <n>     pause for <n> VBLs; <n> = 0 ends the sequence

   YM registers used here: 0/1 = channel A period (12-bit, fine/coarse),
   7 = mixer (a CLEAR bit ENABLES; $FE = tone A only), 8 = channel A volume.

   The sequences must live in static storage, not on the stack: Dosound
   returns immediately and the VBL interrupt keeps reading the buffer. */

#define PERIOD(hz) (125000L / (hz))     /* YM clock 2MHz / 16 */

static char seq[32];

/* Build and fire a simple two-step tone: `hz1` for `d1` VBLs, then `hz2`
   for `d2` VBLs, then silence. hz2 == 0 means a single tone. */
static void beep2(int hz1, unsigned char d1, int hz2, unsigned char d2, char vol) {
    long p1 = PERIOD(hz1);
    int n = 0;
    seq[n++] = 7;  seq[n++] = (char)0xFE;              /* mixer: tone A only */
    seq[n++] = 0;  seq[n++] = (char)(p1 & 0xFF);
    seq[n++] = 1;  seq[n++] = (char)((p1 >> 8) & 0x0F);
    seq[n++] = 8;  seq[n++] = vol;
    seq[n++] = (char)0x82; seq[n++] = (char)d1;
    if (hz2) {
        long p2 = PERIOD(hz2);
        seq[n++] = 0;  seq[n++] = (char)(p2 & 0xFF);
        seq[n++] = 1;  seq[n++] = (char)((p2 >> 8) & 0x0F);
        seq[n++] = (char)0x82; seq[n++] = (char)d2;
    }
    seq[n++] = 8;  seq[n++] = 0;                       /* volume off */
    seq[n++] = (char)0x82; seq[n++] = 0;               /* terminate */
    Dosound(seq);
}

static void beep(int hz, unsigned char d, char vol) {
    beep2(hz, d, 0, 0, vol);
}

void snd_init(void) {
    /* Silence all three channels. (Do NOT route this through beep2 with a
       0 frequency -- PERIOD() would divide by zero and trap.) */
    seq[0] = 8; seq[1] = 0;
    seq[2] = 9; seq[3] = 0;
    seq[4] = 10; seq[5] = 0;
    seq[6] = (char)0x82; seq[7] = 0;
    Dosound(seq);
}

void sfx_card_play(void)        { beep2(880, 2, 1320, 3, 12); }
void sfx_invalid(void)          { beep2(220, 4, 165, 5, 13); }
void sfx_draw(void)             { beep(660, 3, 10); }
void sfx_draw_multi(unsigned char count) { (void)count; beep2(440, 3, 330, 4, 12); }
void sfx_skip(void)             { beep2(523, 3, 262, 6, 13); }
void sfx_reverse(void)          { beep2(392, 3, 587, 4, 12); }
void sfx_uno(void)              { beep2(1047, 4, 1319, 6, 14); }
void sfx_win(void)              { beep2(523, 5, 1047, 10, 15); }
void sfx_challenge_success(void){ beep2(784, 3, 1047, 5, 13); }
void sfx_challenge_fail(void)   { beep2(392, 4, 196, 6, 13); }
