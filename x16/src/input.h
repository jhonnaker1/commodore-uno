#ifndef INPUT_H
#define INPUT_H

#define IN_UP 0x01
#define IN_DOWN 0x02
#define IN_LEFT 0x04
#define IN_RIGHT 0x08
#define IN_FIRE 0x10
#define IN_NONE 0xFF

void input_init(void);
unsigned char joy_state(void);
unsigned char joy_pressed(void);
unsigned char joy_quick_select(void);

#endif
