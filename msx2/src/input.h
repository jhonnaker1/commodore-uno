#ifndef INPUT_H
#define INPUT_H

#define IN_UP 0x01
#define IN_DOWN 0x02
#define IN_LEFT 0x04
#define IN_RIGHT 0x08
#define IN_FIRE 0x10

#define IN_NONE 0xFF

/* Currently-held directions and fire, merged from the cursor keys and the
   joystick port -- the BIOS presents both through one call, so the game
   never has to know which the player is using. */
unsigned char joy_state(void);
/* Edge-triggered: bits that went down since the last call. */
unsigned char joy_pressed(void);
/* Hand slot requested by a quick-play key this frame (0-8 for '1'-'9',
   9 for '0', 10-19 for 'A'-'J'), or IN_NONE. Only valid for the frame
   right after the joy_pressed() call that set it. */
unsigned char joy_quick_select(void);
/* Throw away buffered keystrokes. The MSX BIOS collects keys into a
   type-ahead buffer from its own interrupt handler, so unlike the ports
   that poll a key matrix directly, keys pressed during an animation are
   still queued when it ends -- call this where that would misfire. */
void joy_flush(void);

#endif
