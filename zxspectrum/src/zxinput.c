#include <input.h>
#include "zxinput.h"

/* The 48K Spectrum has no dedicated cursor keys, and digits 1-9/0 are
   already spoken for by the quick-play scheme (same convention as every
   other port here), so movement uses the classic "O/P left/right, Q up"
   keys-as-joystick scheme many commercial Spectrum games used, with
   SPACE or ENTER to play/confirm -- same idea as the Amiga/C128 letter-key
   fallbacks elsewhere in this project, just Spectrum's own well-known one. */

static unsigned char prev_state = 0;
static unsigned char prev_quick_raw = IN_NONE;
static unsigned char quick_select = IN_NONE;

static unsigned char key_state(void) {
    unsigned char state = 0;
    if (in_key_pressed(IN_KEY_SCANCODE_o)) state |= IN_LEFT;
    if (in_key_pressed(IN_KEY_SCANCODE_p)) state |= IN_RIGHT;
    if (in_key_pressed(IN_KEY_SCANCODE_q)) state |= IN_UP;
    if (in_key_pressed(IN_KEY_SCANCODE_SPACE)) state |= IN_FIRE;
    if (in_key_pressed(IN_KEY_SCANCODE_ENTER)) state |= IN_FIRE;
    return state;
}

/* '1'-'9' -> 0-8, '0' -> 9, 'a'-'j' -> 10-19, or IN_NONE if none held. */
static unsigned char quick_select_raw(void) {
    if (in_key_pressed(IN_KEY_SCANCODE_1)) return 0;
    if (in_key_pressed(IN_KEY_SCANCODE_2)) return 1;
    if (in_key_pressed(IN_KEY_SCANCODE_3)) return 2;
    if (in_key_pressed(IN_KEY_SCANCODE_4)) return 3;
    if (in_key_pressed(IN_KEY_SCANCODE_5)) return 4;
    if (in_key_pressed(IN_KEY_SCANCODE_6)) return 5;
    if (in_key_pressed(IN_KEY_SCANCODE_7)) return 6;
    if (in_key_pressed(IN_KEY_SCANCODE_8)) return 7;
    if (in_key_pressed(IN_KEY_SCANCODE_9)) return 8;
    if (in_key_pressed(IN_KEY_SCANCODE_0)) return 9;
    if (in_key_pressed(IN_KEY_SCANCODE_a)) return 10;
    if (in_key_pressed(IN_KEY_SCANCODE_b)) return 11;
    if (in_key_pressed(IN_KEY_SCANCODE_c)) return 12;
    if (in_key_pressed(IN_KEY_SCANCODE_d)) return 13;
    if (in_key_pressed(IN_KEY_SCANCODE_e)) return 14;
    if (in_key_pressed(IN_KEY_SCANCODE_f)) return 15;
    if (in_key_pressed(IN_KEY_SCANCODE_g)) return 16;
    if (in_key_pressed(IN_KEY_SCANCODE_h)) return 17;
    if (in_key_pressed(IN_KEY_SCANCODE_i)) return 18;
    if (in_key_pressed(IN_KEY_SCANCODE_j)) return 19;
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
