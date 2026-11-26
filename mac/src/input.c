/* Keyboard and mouse input for the Mac, taken from the queues the event
   loop in macgfx.c fills. The Mac Plus keyboard has real arrow keys (the
   original 128K/512K keyboard does not), so both they and the comma/period
   pair the Amiga port uses move the cursor. */
#include "input.h"
#include "macgfx.h"
#include "ui.h"

static unsigned char quick_select = IN_NONE;

unsigned char joy_state(void) {
    return 0;
}

static unsigned char from_key(int k) {
    unsigned char mode = ui_mode();
    switch (k) {
    case 0x1C: case ',': case '<': return IN_LEFT;  /* left arrow */
    case 0x1D: case '.': case '>': return IN_RIGHT; /* right arrow */
    case 0x1E: case 'u': case 'U': return IN_UP;    /* up arrow: draw */
    case 0x1F: return IN_DOWN;
    case ' ': case '\r': case 0x03: return IN_FIRE; /* 0x03 = Enter */
    default: break;
    }
    if (mode != UI_MODE_GAME) return 0;
    if (k >= '1' && k <= '9') quick_select = (unsigned char)(k - '1');
    else if (k == '0') quick_select = 9;
    else if (k >= 'A' && k <= 'J') quick_select = (unsigned char)(10 + (k - 'A'));
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
