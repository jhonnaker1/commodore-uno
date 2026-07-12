#include <coco.h>
#include "cocoinput.h"

/* CoCo has real cursor keys and a real joystick port, but the quick-play
   scheme (1-9,0,A-J) already needs a keyboard, so this port is keyboard-
   only like the PET port -- one input path instead of two to test. */

static unsigned char prev_state = 0;
static unsigned char prev_quick_raw = IN_NONE;
static unsigned char quick_select = IN_NONE;

static unsigned char key_state(void) {
    unsigned char state = 0;
    if (isKeyPressed(KEY_PROBE_UP, KEY_BIT_UP)) state |= IN_UP;
    if (isKeyPressed(KEY_PROBE_DOWN, KEY_BIT_DOWN)) state |= IN_DOWN;
    if (isKeyPressed(KEY_PROBE_LEFT, KEY_BIT_LEFT)) state |= IN_LEFT;
    if (isKeyPressed(KEY_PROBE_RIGHT, KEY_BIT_RIGHT)) state |= IN_RIGHT;
    if (isKeyPressed(KEY_PROBE_SPACE, KEY_BIT_SPACE)) state |= IN_FIRE;
    if (isKeyPressed(KEY_PROBE_ENTER, KEY_BIT_ENTER)) state |= IN_FIRE;
    return state;
}

/* '1'-'9' -> 0-8, '0' -> 9, 'a'-'j' -> 10-19, or IN_NONE if none held. */
static unsigned char quick_select_raw(void) {
    if (isKeyPressed(KEY_PROBE_1, KEY_BIT_1)) return 0;
    if (isKeyPressed(KEY_PROBE_2, KEY_BIT_2)) return 1;
    if (isKeyPressed(KEY_PROBE_3, KEY_BIT_3)) return 2;
    if (isKeyPressed(KEY_PROBE_4, KEY_BIT_4)) return 3;
    if (isKeyPressed(KEY_PROBE_5, KEY_BIT_5)) return 4;
    if (isKeyPressed(KEY_PROBE_6, KEY_BIT_6)) return 5;
    if (isKeyPressed(KEY_PROBE_7, KEY_BIT_7)) return 6;
    if (isKeyPressed(KEY_PROBE_8, KEY_BIT_8)) return 7;
    if (isKeyPressed(KEY_PROBE_9, KEY_BIT_9)) return 8;
    if (isKeyPressed(KEY_PROBE_0, KEY_BIT_0)) return 9;
    if (isKeyPressed(KEY_PROBE_A, KEY_BIT_A)) return 10;
    if (isKeyPressed(KEY_PROBE_B, KEY_BIT_B)) return 11;
    if (isKeyPressed(KEY_PROBE_C, KEY_BIT_C)) return 12;
    if (isKeyPressed(KEY_PROBE_D, KEY_BIT_D)) return 13;
    if (isKeyPressed(KEY_PROBE_E, KEY_BIT_E)) return 14;
    if (isKeyPressed(KEY_PROBE_F, KEY_BIT_F)) return 15;
    if (isKeyPressed(KEY_PROBE_G, KEY_BIT_G)) return 16;
    if (isKeyPressed(KEY_PROBE_H, KEY_BIT_H)) return 17;
    if (isKeyPressed(KEY_PROBE_I, KEY_BIT_I)) return 18;
    if (isKeyPressed(KEY_PROBE_J, KEY_BIT_J)) return 19;
    return IN_NONE;
}

unsigned char joy_quick_select(void) {
    return quick_select;
}

/* Edge-triggered: a bit is set only on the frame a key was newly pressed,
   so holding a key down doesn't repeat-fire every frame. */
unsigned char joy_pressed(void) {
    unsigned char cur = key_state();
    unsigned char newly = cur & (unsigned char)~prev_state;
    unsigned char raw;
    prev_state = cur;

    raw = quick_select_raw();
    quick_select = (raw != IN_NONE && raw != prev_quick_raw) ? raw : IN_NONE;
    prev_quick_raw = raw;

    return newly;
}
