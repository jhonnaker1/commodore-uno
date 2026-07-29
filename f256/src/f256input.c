#include <stddef.h>
#include "input.h"
#include "api.h"

/* Keyboard input via the FoenixMCP kernel event queue. The kernel fills the
   queue from its keyboard IRQ; NextEvent pulls the next event non-blocking
   (carry set = queue empty), so joy_pressed() polls like every other port's
   joystick read rather than blocking. crt0 allocates `event` (zero page) and
   points the kernel's event descriptor at it.

   Controls are letter/punctuation keys, all of which produce a definite
   ASCII code: ',' / '.' move, space or return plays, 'U' draws, 1-9/0/A-J
   jump to a hand slot. */

extern struct event_t event;
#pragma zpsym ("event")

#define VECTOR(m) (size_t)(&((struct call *)0xff00)->m)
#define EVENT(m)  (size_t)(&((struct events *)0)->m)

static unsigned char nb_err;
static unsigned char quick_select = IN_NONE;

/* ASCII of a key pressed this call, or 0 if the event queue held no key. */
static unsigned char poll_key(void) {
    __asm__("jsr %w", VECTOR(NextEvent));
    __asm__("stz %v", nb_err);
    __asm__("ror %v", nb_err);       /* carry -> bit 7: nonzero == no event */
    if (nb_err) return 0;
    if (event.type != EVENT(key.PRESSED)) return 0;
    if (event.key.flags) return 0;   /* meta key / no ASCII */
    return (unsigned char)event.key.ascii;
}

unsigned char joy_state(void) {
    return 0;
}

unsigned char joy_quick_select(void) {
    return quick_select;
}

unsigned char joy_pressed(void) {
    unsigned char c = poll_key();
    unsigned char newly = 0;
    quick_select = IN_NONE;
    if (!c) return 0;

    switch (c) {
        case ',': newly |= IN_LEFT; break;
        case '.': newly |= IN_RIGHT; break;
        case ' ':
        case '\r':
        case '\n': newly |= IN_FIRE; break;
        case 'u':
        case 'U': newly |= IN_UP; break;
        default:
            if (c >= '1' && c <= '9') quick_select = (unsigned char)(c - '1');
            else if (c == '0') quick_select = 9;
            else if (c >= 'A' && c <= 'J') quick_select = (unsigned char)(10 + (c - 'A'));
            else if (c >= 'a' && c <= 'j') quick_select = (unsigned char)(10 + (c - 'a'));
            break;
    }
    return newly;
}
