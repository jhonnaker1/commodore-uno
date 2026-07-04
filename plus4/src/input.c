#include <conio.h>
#include <cbm.h>
#include <joystick.h>
#include "input.h"

static unsigned char prev_state = 0;
static unsigned char quick_select = IN_NONE;

void input_init(void) {
    joy_install(joy_static_stddrv);
}

unsigned char joy_state(void) {
    unsigned char raw = joy_read(JOY_1);
    unsigned char out = 0;
    if (JOY_UP(raw)) out |= IN_UP;
    if (JOY_DOWN(raw)) out |= IN_DOWN;
    if (JOY_LEFT(raw)) out |= IN_LEFT;
    if (JOY_RIGHT(raw)) out |= IN_RIGHT;
    if (JOY_BTN_1(raw)) out |= IN_FIRE;
    return out;
}

unsigned char joy_quick_select(void) {
    return quick_select;
}

/* Edge-triggered: joystick OR the keyboard (cursor keys + space/return +
   '1'-'9'/'0'/'A'-'J' quick-play). */
unsigned char joy_pressed(void) {
    unsigned char cur = joy_state();
    unsigned char newly = cur & (unsigned char)~prev_state;
    prev_state = cur;
    quick_select = IN_NONE;

    if (kbhit()) {
        char k = cgetc();
        switch (k) {
            case CH_CURS_LEFT:
                newly |= IN_LEFT;
                break;
            case CH_CURS_RIGHT:
                newly |= IN_RIGHT;
                break;
            case CH_CURS_UP:
                newly |= IN_UP;
                break;
            case CH_ENTER:
            case ' ':
                newly |= IN_FIRE;
                break;
            default:
                if (k >= '1' && k <= '9') {
                    quick_select = (unsigned char)(k - '1');
                } else if (k == '0') {
                    quick_select = 9;
                } else if (k >= 'A' && k <= 'J') {
                    quick_select = (unsigned char)(10 + (k - 'A'));
                } else if (k >= 'a' && k <= 'j') {
                    quick_select = (unsigned char)(10 + (k - 'a'));
                }
                break;
        }
    }
    return newly;
}
