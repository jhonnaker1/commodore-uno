#include <vdp.h>
#include <kscan.h>
#include "tiinput.h"

/* Keyboard through the console's own SCAN routine (KSCAN), which handles the
   CRU row/column scan and debounce for us and hands back one ASCII key in
   KSCAN_KEY, or 0xFF for none.

   The TI-99/4A has no dedicated cursor keys -- the arrows are FCTN+S/D/E/X
   and come back as control codes rather than plain ASCII -- so movement uses
   the comma/period scheme this project already uses on the Amiga and F256
   ports, with 'U' to draw. Digits and A-J are spoken for by the quick-play
   keys (same convention as every other port here), which is also why the
   draw key is 'U' and not the more obvious 'D'. */

static unsigned char prev_key = 0xFF;

static unsigned char scan_key(void) {
    kscan(KSCAN_MODE_BASIC);
    return KSCAN_KEY;
}

/* One key at a time (the console scan is not a matrix snapshot), reported
   only on the transition into a new key so holding one down doesn't
   repeat-fire every frame. */
static unsigned char newly_pressed(unsigned char *out_key) {
    unsigned char key = scan_key();
    unsigned char is_new = (key != 0xFF && key != prev_key);
    prev_key = key;
    *out_key = key;
    return is_new;
}

static unsigned char pending_quick = IN_NONE;

unsigned char joy_quick_select(void) {
    return pending_quick;
}

unsigned char joy_pressed(void) {
    unsigned char key;
    unsigned char state = 0;

    pending_quick = IN_NONE;
    if (!newly_pressed(&key)) return 0;

    switch (key) {
        case ',': state = IN_LEFT; break;
        case '.': state = IN_RIGHT; break;
        case ' ':
        case 13:  state = IN_FIRE; break;
        case 'U':
        case 'u': state = IN_UP; break;
        default:
            if (key >= '1' && key <= '9') {
                pending_quick = (unsigned char)(key - '1');
            } else if (key == '0') {
                pending_quick = 9;
            } else if (key >= 'A' && key <= 'J') {
                pending_quick = (unsigned char)(10 + (key - 'A'));
            } else if (key >= 'a' && key <= 'j') {
                pending_quick = (unsigned char)(10 + (key - 'a'));
            }
            break;
    }
    return state;
}
