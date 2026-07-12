#include <coco.h>
#include "cocosound.h"

/* coco.h's sound(tone, duration) is a direct passthrough to Color Basic's
   SOUND statement (blocking, like the other simple-speaker ports here),
   so it requires Color Basic to still be resident -- true by default,
   since this compiles for the Disk Basic environment (--dos/--coco) and
   nothing here replaces Basic. Exact tone/duration units are tuned by
   ear once heard running; see the README's hardware notes. */

void snd_init(void) {
}

void sfx_card_play(void) {
    sound(200, 2);
}

void sfx_invalid(void) {
    sound(20, 8);
}

void sfx_draw(void) {
    sound(120, 2);
}

void sfx_draw_multi(unsigned char count) {
    unsigned char i;
    if (count > 4) count = 4;
    for (i = 0; i < count; i++) {
        sound(120, 2);
    }
}

void sfx_skip(void) {
    sound(80, 3);
    sound(60, 3);
}

void sfx_reverse(void) {
    sound(100, 2);
    sound(140, 2);
    sound(100, 2);
}

void sfx_uno(void) {
    sound(150, 3);
    sound(180, 3);
    sound(220, 4);
}

void sfx_win(void) {
    sound(150, 3);
    sound(180, 3);
    sound(220, 3);
    sound(250, 6);
}

void sfx_challenge_success(void) {
    sound(220, 3);
    sound(250, 6);
}

void sfx_challenge_fail(void) {
    sound(80, 3);
    sound(30, 8);
}
