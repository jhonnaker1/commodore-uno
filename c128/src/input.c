#include <6502.h>
#include "input.h"

#define CIA1_PRA (*(unsigned char *)0xDC00)
#define CIA1_PRB (*(unsigned char *)0xDC01)

/* The C128's dedicated cursor keys turned out to be unreachable both by
   a raw CIA1 matrix scan (confirmed empirically: no bit anywhere in
   PRA/PRB, in either column/row orientation, changes when they're
   pressed -- they live outside the standard 8x8 matrix used here) and
   by the KERNAL's own keyboard buffer (kbhit()/cgetc() go completely
   dead once the screen is relocated to VIC bank 2, for reasons that
   resisted several fix attempts, including correcting the KERNAL's
   cursor-tracking zero page variables). So navigation is remapped to
   ordinary matrix keys that ARE confirmed working: comma/period for
   left/right (they sit right next to space on this keyboard) and 'U'
   for draw. Positions below were looked up directly from VICE's own
   C128 keymap file rather than trusted from a "standard" reference
   table, since that reference table was already wrong once this
   session for several other keys. */

typedef struct { unsigned char col, row; } KeyPos;

/* index: 0=space 1=return 2=comma(left) 3=period(right) 4=u(draw),
   5-14='1'-'9','0', 15-24='A'-'J'. */
static const KeyPos KEYS[25] = {
    {7, 4}, {0, 1}, {5, 7}, {5, 4}, {3, 6},
    {7, 0}, {7, 3}, {1, 0}, {1, 3}, {2, 0}, {2, 3}, {3, 0}, {3, 3}, {4, 0}, {4, 3},
    {1, 2}, {3, 4}, {2, 4}, {2, 2}, {1, 6}, {2, 5}, {3, 2}, {3, 5}, {4, 1}, {4, 2}
};

static unsigned char prev_joy = 0;
static unsigned char prev_special = 0;
static unsigned char prev_qs_active = 0;
static unsigned char quick_select = IN_NONE;

/* The KERNAL's own 60Hz IRQ is still running (we never disabled it) and
   does its own column-select/row-read of CIA1 for cursor blink. If that
   IRQ lands between our column write and row read, it clobbers the
   column we just selected -- so the whole read has to happen with
   interrupts off. */
static unsigned char key_down(unsigned char idx) {
    unsigned char save;
    unsigned char pressed;
    SEI();
    save = CIA1_PRA;
    CIA1_PRA = (unsigned char)~(1 << KEYS[idx].col);
    pressed = (CIA1_PRB & (1 << KEYS[idx].row)) == 0;
    CIA1_PRA = save;
    CLI();
    return pressed;
}

unsigned char joy_state(void) {
    unsigned char raw;
    SEI();
    raw = ~CIA1_PRA; /* joystick port 2, active low */
    CLI();
    return raw & 0x1F;
}

unsigned char joy_quick_select(void) {
    return quick_select;
}

/* Edge-triggered: joystick port 2 OR the keyboard (comma/period/U for
   left/right/draw, space/return to play, '1'-'9'/'0'/'A'-'J' quick-play),
   so the game is playable with no joystick configured at all. */
unsigned char joy_pressed(void) {
    unsigned char cur_joy = joy_state();
    unsigned char newly = cur_joy & (unsigned char)~prev_joy;
    unsigned char cur_special = 0;
    unsigned char i;
    unsigned char qs_down = IN_NONE;

    prev_joy = cur_joy;
    quick_select = IN_NONE;

    if (key_down(0) || key_down(1)) cur_special |= IN_FIRE;
    if (key_down(2)) cur_special |= IN_LEFT;
    if (key_down(3)) cur_special |= IN_RIGHT;
    if (key_down(4)) cur_special |= IN_UP;
    newly |= cur_special & (unsigned char)~prev_special;
    prev_special = cur_special;

    for (i = 5; i < 25; i++) {
        if (key_down(i)) {
            qs_down = (unsigned char)(i - 5);
            break;
        }
    }
    if (qs_down != IN_NONE && !prev_qs_active) {
        quick_select = qs_down;
    }
    prev_qs_active = (qs_down != IN_NONE);

    return newly;
}
