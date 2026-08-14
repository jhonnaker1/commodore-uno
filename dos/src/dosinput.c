#include <conio.h>
#include "input.h"

/* Keyboard through the C library's kbhit()/getch(), which sit directly on
   INT 16h. The PC keyboard controller only reports make/break scancodes via
   an interrupt -- there is no key-matrix port to poll the way the CoCo,
   Spectrum and C64 ports do -- so the BIOS's handler and its ring buffer
   are what we read. Input here is therefore inherently edge-triggered: we
   see keystrokes, never "is this key down right now".

   Not int86() with INT 16h AH=01h, which is the obvious way to write this
   and is wrong: that call reports "a key is waiting" in the *zero* flag,
   and union REGS only carries the carry flag back. kbhit() is the same
   BIOS call with the flag handled properly.

   An extended key (the arrows, function keys) arrives as two reads: a 0 or
   0xE0 lead byte, then the scancode. */

#define SC_UP     0x48
#define SC_LEFT   0x4B
#define SC_RIGHT  0x4D
#define SC_DOWN   0x50

static unsigned char quick;
static unsigned char quit_flag;

unsigned char joy_pressed(void)
{
    unsigned char bits = 0;
    int c;

    quick = IN_NONE;
    while (kbhit()) {
        c = getch();

        if (c == 0 || c == 0xE0) {
            switch (getch()) {
            case SC_LEFT:  bits |= IN_LEFT;  break;
            case SC_RIGHT: bits |= IN_RIGHT; break;
            case SC_UP:    bits |= IN_UP;    break;
            case SC_DOWN:  bits |= IN_DOWN;  break;
            default: break;
            }
            continue;
        }

        if (c == ' ' || c == 13) bits |= IN_FIRE;
        else if (c == 'U' || c == 'u') bits |= IN_UP;
        else if (c == 27) quit_flag = 1;
        else if (c >= '1' && c <= '9') quick = (unsigned char)(c - '1');
        else if (c == '0') quick = 9;
        else if (c >= 'A' && c <= 'J') quick = (unsigned char)(c - 'A' + 10);
        else if (c >= 'a' && c <= 'j') quick = (unsigned char)(c - 'a' + 10);
    }
    return bits;
}

unsigned char joy_state(void)
{
    return joy_pressed();
}

unsigned char joy_quick_select(void)
{
    return quick;
}

void joy_flush(void)
{
    while (kbhit()) getch();
    quick = IN_NONE;
}

unsigned char joy_quit_requested(void)
{
    return quit_flag;
}
