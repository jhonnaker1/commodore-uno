#include <sound.h>
#include "zxsound.h"

/* 1-bit beeper via z88dk's bit_beep(ms, hz) -- blocking, like the other
   simple-speaker ports (PET/Apple/Atari), but with real Hz/ms parameters
   instead of a hand-tuned delay loop since bit_beep does that math for us. */

void snd_init(void) {
}

void sfx_card_play(void) {
    bit_beep(40, 800);
}

void sfx_invalid(void) {
    bit_beep(150, 150);
}

void sfx_draw(void) {
    bit_beep(40, 500);
}

void sfx_draw_multi(unsigned char count) {
    unsigned char i;
    if (count > 4) count = 4;
    for (i = 0; i < count; i++) {
        bit_beep(40, 500);
    }
}

void sfx_skip(void) {
    bit_beep(60, 300);
    bit_beep(70, 200);
}

void sfx_reverse(void) {
    bit_beep(50, 400);
    bit_beep(50, 300);
    bit_beep(50, 400);
}

void sfx_uno(void) {
    bit_beep(50, 600);
    bit_beep(50, 800);
    bit_beep(70, 1000);
}

void sfx_win(void) {
    bit_beep(50, 500);
    bit_beep(50, 650);
    bit_beep(50, 800);
    bit_beep(100, 1000);
}

void sfx_challenge_success(void) {
    bit_beep(50, 700);
    bit_beep(90, 1000);
}

void sfx_challenge_fail(void) {
    bit_beep(50, 300);
    bit_beep(120, 150);
}
