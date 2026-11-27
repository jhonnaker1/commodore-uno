#ifndef INPUT_H
#define INPUT_H

#define IN_UP 0x01
#define IN_DOWN 0x02
#define IN_LEFT 0x04
#define IN_RIGHT 0x08
#define IN_FIRE 0x10

#define IN_NONE 0xFF

/* No continuous "currently held" source: keys and clicks arrive as
   discrete AppKit events. Kept so the interface matches every port. */
unsigned char joy_state(void);
/* Edge-triggered: the key or click taken from the event queue this frame. */
unsigned char joy_pressed(void);
/* What the same key or click selected directly, or IN_NONE: a hand slot
   during play ('1'-'9','0','A'-'J', or a clicked card), or -- clicks only --
   a colour 0-3 in the colour picker, or 1 = Yes / 0 = No at a challenge.
   Only valid right after the joy_pressed() call that set it. */
unsigned char joy_quick_select(void);

#endif
