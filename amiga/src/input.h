#ifndef INPUT_H
#define INPUT_H

#define IN_UP 0x01
#define IN_DOWN 0x02
#define IN_LEFT 0x04
#define IN_RIGHT 0x08
#define IN_FIRE 0x10

#define IN_NONE 0xFF

/* No continuous "currently held" input source on this platform (keyboard
   events from the CON: handle are discrete keypresses, not a pollable
   held-key bitmask the way a joystick port is) -- kept only so the
   interface matches every other port's input.h. */
unsigned char joy_state(void);
/* Edge-triggered: bit set for the key event read this frame (cursor
   keys, space/return). */
unsigned char joy_pressed(void);
/* Hand slot requested by a keyboard quick-play key this frame (0-8 for '1'-'9',
   9 for '0', 10-19 for 'A'-'J'), or IN_NONE if no such key was pressed. Only
   valid for the frame right after the joy_pressed() call that set it. */
unsigned char joy_quick_select(void);

#endif
