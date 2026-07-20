#include <tos.h>
#include "input.h"

/* ST/STE input. Reads the keyboard through the BIOS console (device 2),
   whose Bconin() return packs the raw scancode in bits 16-23 and the ASCII
   value in bits 0-7 -- we need the scancode because the cursor keys have no
   ASCII code. Bconstat() makes the poll non-blocking, so this slots into the
   game loop the same way the other ports' joystick reads do.

   Keyboard only: the ST's joystick ports are behind the IKBD, which reports
   sticks only after switching it out of mouse mode and servicing its packets
   on an interrupt vector. That's a fair amount of machinery for a turn-based
   card game, and Hatari (like most ST setups) can map the cursor keys to a
   joystick anyway, so the cursor keys are the control scheme here. */

#define SC_UP 0x48
#define SC_DOWN 0x50
#define SC_LEFT 0x4B
#define SC_RIGHT 0x4D

static unsigned char quick_select = IN_NONE;

/* No analogue/held state to report: the keyboard is edge-driven, so
   joy_state() exists only to satisfy the shared input.h contract. */
unsigned char joy_state(void) {
    return 0;
}

unsigned char joy_quick_select(void) {
    return quick_select;
}

unsigned char joy_pressed(void) {
    unsigned char newly = 0;
    quick_select = IN_NONE;

    while (Bconstat(2)) {
        long k = Bconin(2);
        unsigned char scan = (unsigned char)((k >> 16) & 0xFF);
        unsigned char ch = (unsigned char)(k & 0xFF);

        switch (scan) {
            case SC_LEFT:  newly |= IN_LEFT;  continue;
            case SC_RIGHT: newly |= IN_RIGHT; continue;
            case SC_UP:    newly |= IN_UP;    continue;
            case SC_DOWN:  newly |= IN_DOWN;  continue;
            default: break;
        }
        if (ch == ' ' || ch == '\r' || ch == '\n') {
            newly |= IN_FIRE;
        } else if (ch >= '1' && ch <= '9') {
            quick_select = (unsigned char)(ch - '1');
        } else if (ch == '0') {
            quick_select = 9;
        } else if (ch >= 'A' && ch <= 'J') {
            quick_select = (unsigned char)(10 + (ch - 'A'));
        } else if (ch >= 'a' && ch <= 'j') {
            quick_select = (unsigned char)(10 + (ch - 'a'));
        }
    }
    return newly;
}
