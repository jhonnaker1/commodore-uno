#ifndef INPUT_H
#define INPUT_H

#define IN_UP 0x01
#define IN_DOWN 0x02
#define IN_LEFT 0x04
#define IN_RIGHT 0x08
#define IN_FIRE 0x10

#define IN_NONE 0xFF

/* The PC keyboard has no pollable "currently held" state through the BIOS --
   INT 16h reports keystrokes from a buffer, not the key matrix -- so this
   returns the same edge-triggered value as joy_pressed(). Kept so the
   interface matches every other port's input.h. */
unsigned char joy_state(void);
/* Edge-triggered: keys that arrived since the last call. Real arrow keys,
   unlike most ports here, plus SPACE/RETURN to confirm. */
unsigned char joy_pressed(void);
/* Hand slot requested by a quick-play key this frame (0-8 for '1'-'9',
   9 for '0', 10-19 for 'A'-'J'), or IN_NONE. */
unsigned char joy_quick_select(void);
/* Drop anything still sitting in the BIOS keyboard buffer, so keys pressed
   during an animation don't fire an action when it ends. */
void joy_flush(void);
/* True if ESC was seen -- the DOS ports get a way out to the prompt, which
   the cartridge and disk ports have no equivalent for. */
unsigned char joy_quit_requested(void);

#endif
