#include "input.h"

/* Input on the MSX comes from two places at once and the game does not care
   which: the BIOS reads the cursor keys and a real joystick through the
   same call, so this port supports both without special-casing either.
   That is a small luxury -- most machines in this repo have one or the
   other, and the ones with a joystick port need it wired up separately. */

extern unsigned char bios_gtstck(unsigned char stick);
extern unsigned char bios_gttrig(unsigned char trig);
extern unsigned char bios_chsns(void);
extern unsigned char bios_chget(void);
extern void bios_kilbuf(void);

/* GTSTCK returns 0 for centred and 1-8 clockwise from up, so a diagonal is
   a single value rather than two bits -- this maps it back to the bitmask
   the shared game loop expects. Index 0 is the centred case. */
static const unsigned char stick_bits[9] = {
    0,
    IN_UP,
    IN_UP | IN_RIGHT,
    IN_RIGHT,
    IN_DOWN | IN_RIGHT,
    IN_DOWN,
    IN_DOWN | IN_LEFT,
    IN_LEFT,
    IN_UP | IN_LEFT
};

static unsigned char last_state;
static unsigned char quick;

unsigned char joy_state(void) {
    unsigned char s;
    unsigned char d;

    /* Stick 0 is the cursor keys; stick 1 is the first joystick port. */
    d = bios_gtstck(0);
    s = (d <= 8) ? stick_bits[d] : 0;
    d = bios_gtstck(1);
    if (d <= 8) s |= stick_bits[d];

    /* Trigger 0 is the space bar, trigger 1 the joystick button. */
    if (bios_gttrig(0) || bios_gttrig(1)) s |= IN_FIRE;
    return s;
}

/* Edge-triggered: only bits that were not set last time. Without this a
   held cursor key would run the hand cursor off the end in three frames. */
unsigned char joy_pressed(void) {
    unsigned char now = joy_state();
    unsigned char newly = (unsigned char)(now & ~last_state);
    last_state = now;

    /* Drain the type-ahead buffer in the same pass, so a quick-play key is
       reported alongside the direction bits for this frame. The BIOS fills
       that buffer from its own interrupt handler, so keys pressed during an
       animation are still waiting here rather than lost. */
    quick = IN_NONE;
    while (bios_chsns()) {
        unsigned char c = bios_chget();
        if (c >= '1' && c <= '9') quick = (unsigned char)(c - '1');
        else if (c == '0') quick = 9;
        else if (c >= 'A' && c <= 'J') quick = (unsigned char)(c - 'A' + 10);
        else if (c >= 'a' && c <= 'j') quick = (unsigned char)(c - 'a' + 10);
        /* U draws a card, matching the other ports' keyboard layout; the
           cursor-up key does the same thing and is the obvious one to
           reach for here, so both are folded into IN_UP. */
        else if (c == 'U' || c == 'u') newly |= IN_UP;
        /* RETURN confirms as well as SPACE. The joystick trigger and the
           space bar are the same button as far as GTTRIG is concerned, so
           space is the port's natural confirm -- but at a YES/NO prompt
           RETURN is the key a player actually reaches for, and having it do
           nothing reads as the game being frozen. */
        else if (c == ' ' || c == 13) newly |= IN_FIRE;
    }
    return newly;
}

unsigned char joy_quick_select(void) {
    return quick;
}

/* Called before handing control back to the player after an animation, so a
   key held down through it does not immediately fire an action. */
void joy_flush(void) {
    bios_kilbuf();
    last_state = joy_state();
    quick = IN_NONE;
}
