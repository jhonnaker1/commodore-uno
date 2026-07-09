#ifndef ZXINPUT_H
#define ZXINPUT_H

#define IN_UP 0x01
#define IN_DOWN 0x02
#define IN_LEFT 0x04
#define IN_RIGHT 0x08
#define IN_FIRE 0x10

#define IN_NONE 0xFF

/* Edge-triggered: bit set only on the frame the key was newly pressed. */
unsigned char joy_pressed(void);
/* Hand slot requested by a quick-play digit key this frame (0-8 for '1'-'9',
   9 for '0', 10-19 for 'A'-'J'), or IN_NONE if no such key was pressed. */
unsigned char joy_quick_select(void);

#endif
