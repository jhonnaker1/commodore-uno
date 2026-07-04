#include <conio.h>
#include <cbm.h>
#include "input.h"

static unsigned char quick_select = IN_NONE;

void input_init(void) {
}

unsigned char joy_quick_select(void) {
    return quick_select;
}

unsigned char joy_pressed(void) {
    unsigned char newly = 0;

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
