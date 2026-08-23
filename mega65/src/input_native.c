#include "input.h"

/* Native-mode input. The C64-mode build (input.c) uses cc65's <conio.h>,
   <cbm.h> and <joystick.h>; none of those exist under llvm-mos, so this is
   a direct-hardware rewrite -- which turns out to be the simpler of the two.

   Keyboard comes from $D610, the MEGA65's ASCII key register: reading it
   gives the ASCII code of the key waiting (0 when the buffer is empty), and
   writing any value pops that entry. That is a genuine native-mode luxury.
   The C64-mode build has to go through the KERNAL's PETSCII buffer and
   translate; here the hardware hands over ASCII directly, so 'A' really is
   0x41 and no charmap is involved anywhere in this port.

   Joystick is the ordinary CIA1 port A at $DC00 (control port 2), active
   low -- unchanged from the C64, since the MEGA65 keeps both CIAs. */
#define ASCIIKEY  (*(volatile unsigned char *)0xD610)
#define CIA1_PRA  (*(volatile unsigned char *)0xDC00)

/* $D610 ASCII codes for the cursor keys (these match PETSCII's control
   codes, which is where the MEGA65 inherited them from). */
#define KEY_CRSR_RIGHT 0x1D
#define KEY_CRSR_LEFT  0x9D
#define KEY_CRSR_DOWN  0x11
#define KEY_CRSR_UP    0x91
#define KEY_RETURN     0x0D

static unsigned char prev_state = 0;
static unsigned char quick_select = IN_NONE;

void input_init(void) {
    ASCIIKEY = 0;   /* drop anything typed before the game started */
}

unsigned char joy_state(void) {
    unsigned char raw = CIA1_PRA;
    unsigned char out = 0;
    if (!(raw & 0x01)) out |= IN_UP;
    if (!(raw & 0x02)) out |= IN_DOWN;
    if (!(raw & 0x04)) out |= IN_LEFT;
    if (!(raw & 0x08)) out |= IN_RIGHT;
    if (!(raw & 0x10)) out |= IN_FIRE;
    return out;
}

unsigned char joy_quick_select(void) {
    return quick_select;
}

/* Edge-triggered: joystick OR the keyboard (cursor keys, comma/period,
   space/return, and '1'-'9'/'0'/'A'-'J' quick-play). */
unsigned char joy_pressed(void) {
    unsigned char cur = joy_state();
    unsigned char newly = cur & (unsigned char)~prev_state;
    unsigned char k;
    prev_state = cur;
    quick_select = IN_NONE;

    k = ASCIIKEY;
    if (k) {
        ASCIIKEY = 0;   /* pop it */
        switch (k) {
            case KEY_CRSR_LEFT:
            case ',':
                newly |= IN_LEFT;
                break;
            case KEY_CRSR_RIGHT:
            case '.':
                newly |= IN_RIGHT;
                break;
            case KEY_CRSR_UP:
            case 'U':
            case 'u':
                newly |= IN_UP;
                break;
            case KEY_CRSR_DOWN:
                newly |= IN_DOWN;
                break;
            case KEY_RETURN:
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
