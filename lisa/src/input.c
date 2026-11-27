/* Keyboard and mouse input for the Lisa, from the queues lisagfx.c fills
   from the COPS. A click goes through ui_hit(), as on the Mac. The Lisa keyboard has no arrow keys, so the cursor moves with
   comma and period (as on the Amiga) or the numeric keypad's 4 and 6, and
   U or keypad 8 draws. */
#include "input.h"
#include "lisagfx.h"
#include "ui.h"

static unsigned char quick_select = IN_NONE;

unsigned char joy_state(void) {
    return 0;
}

static unsigned char from_key(int k) {
    switch (k) {
    case 0x1C: case ',': return IN_LEFT;      /* keypad 4 */
    case 0x1D: case '.': return IN_RIGHT;     /* keypad 6 */
    case 0x1E: case 'u': return IN_UP;        /* keypad 8: draw */
    case 0x1F: return IN_DOWN;                /* keypad 2 */
    case ' ': case '\r': case 0x03: return IN_FIRE;
    default: break;
    }
    if (ui_mode() != UI_MODE_GAME) return 0;
    if (k >= '1' && k <= '9') quick_select = (unsigned char)(k - '1');
    else if (k == '0') quick_select = 9;
    else if (k >= 'a' && k <= 'j') quick_select = (unsigned char)(10 + (k - 'a'));
    return 0;
}

static unsigned char from_click(short x, short y) {
    unsigned char hit = ui_hit(x, y);
    if (hit == HIT_NONE) return 0;
    if (hit == HIT_FIRE) return IN_FIRE;
    if (hit == HIT_DRAW) return IN_UP;
    quick_select = hit;
    return 0;
}

unsigned char joy_pressed(void) {
    short x, y;
    int k;
    quick_select = IN_NONE;
    if (gfx_next_click(&x, &y)) return from_click(x, y);
    k = gfx_next_key();
    if (k) return from_key(k);
    return 0;
}

unsigned char joy_quick_select(void) {
    return quick_select;
}
