#include <6502.h>
#include "input.h"

#define CIA1_PRA (*(unsigned char *)0xDC00)
#define CIA1_PRB (*(unsigned char *)0xDC01)

/* The C128's KERNAL IRQ (which normally fills the keyboard buffer that
   kbhit()/cgetc() read from) stops delivering real key presses once we
   relocate the VIC-IIe screen to bank 2 -- everything else keeps running
   fine, just the keyboard buffer goes dead. Rather than chase that, we
   scan the keyboard matrix directly through CIA1, exactly like the
   joystick read below, so keyboard input never depends on the KERNAL
   IRQ or its buffer at all. */

typedef struct { unsigned char col, row; } KeyPos;

/* index: 0=space 1=return 2=crsr L/R 3=crsr U/D 4=l-shift 5=r-shift,
   6-15='1'-'9','0', 16-25='A'-'J'.
   Verified empirically against the real matrix (via a live on-screen
   scan while pressing each key in VICE) rather than trusted from memory
   -- the previous table had every (col,row) pair transposed from what
   this hardware/emulation actually reports (e.g. space came back as
   col 7/row 4, not the col 4/row 7 the "standard" C64 reference table
   implied), which is why no key ever registered. */
static const KeyPos KEYS[26] = {
    {7, 4}, {0, 1}, {0, 2}, {0, 7}, {1, 7}, {6, 4},
    {7, 0}, {7, 3}, {1, 0}, {1, 3}, {2, 0}, {2, 3}, {3, 0}, {3, 3}, {4, 0}, {4, 3},
    {1, 2}, {3, 4}, {2, 4}, {2, 2}, {1, 6}, {2, 5}, {3, 2}, {3, 5}, {4, 1}, {4, 2}
};

static unsigned char prev_joy = 0;
static unsigned char prev_special = 0;
static unsigned char prev_qs_active = 0;
static unsigned char quick_select = IN_NONE;

/* The KERNAL's own 60Hz IRQ is still running (we never disabled it) and
   does its own column-select/row-read of CIA1 for cursor blink and its
   now-unused keyboard buffer. If that IRQ lands between our column write
   and row read, it clobbers the column we just selected -- so the whole
   read has to happen with interrupts off. */
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

/* Edge-triggered: joystick port 2 OR the keyboard (cursor keys + space/return
   + '1'-'9'/'0'/'A'-'J' quick-play), so the game is playable with no
   joystick configured at all. */
unsigned char joy_pressed(void) {
    unsigned char cur_joy = joy_state();
    unsigned char newly = cur_joy & (unsigned char)~prev_joy;
    unsigned char shift;
    unsigned char cur_special = 0;
    unsigned char i;
    unsigned char qs_down = IN_NONE;

    prev_joy = cur_joy;
    quick_select = IN_NONE;

    shift = key_down(4) || key_down(5);

    if (key_down(0) || key_down(1)) cur_special |= IN_FIRE;
    if (key_down(2)) cur_special |= shift ? IN_LEFT : IN_RIGHT;
    if (key_down(3)) cur_special |= shift ? IN_UP : IN_DOWN;
    newly |= cur_special & (unsigned char)~prev_special;
    prev_special = cur_special;

    for (i = 6; i < 26; i++) {
        if (key_down(i)) {
            qs_down = (unsigned char)(i - 6);
            break;
        }
    }
    if (qs_down != IN_NONE && !prev_qs_active) {
        quick_select = qs_down;
    }
    prev_qs_active = (qs_down != IN_NONE);

    return newly;
}
