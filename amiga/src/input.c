#include "input.h"
#include "amigacon.h"

static unsigned char quick_select = IN_NONE;

unsigned char joy_state(void) {
    return 0;
}

unsigned char joy_pressed(void) {
    int k = con_getkey();
    unsigned char out = 0;
    quick_select = IN_NONE;

    if (k < 0) return 0;

    /* Cursor keys arrive as ANSI escape sequences that turned out not to
       work reliably here (confirmed empirically -- same class of problem
       as the C128's dedicated cursor keys being unreachable, see that
       port's input.c), so movement is remapped to ordinary matrix keys
       instead: comma/period for left/right, 'U' for draw -- same fallback
       used there. */
    switch (k) {
        case ',':
            out |= IN_LEFT;
            break;
        case '.':
            out |= IN_RIGHT;
            break;
        case 'u':
        case 'U':
            out |= IN_UP;
            break;
        case ' ':
        case '\r':
        case '\n':
            out |= IN_FIRE;
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
    return out;
}

unsigned char joy_quick_select(void) {
    return quick_select;
}
