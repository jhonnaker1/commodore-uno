#ifndef MACGFX_H
#define MACGFX_H

/* The Macintosh platform layer: Toolbox start-up, the menu bar, the game
   window, an offscreen 1-bit bitmap the UI draws into, and the event loop.

   Every other port's game loop is a busy loop that calls wait_vsync() once a
   frame. On the Mac that one function is where the event loop lives: it
   pulls events (keys, clicks, menu picks, window drags, desk accessories,
   update events) until the 1/60 s tick counter moves on, then copies the
   offscreen bitmap to the window if anything was drawn. So the shared game
   loop never needs to know it is running under a cooperative GUI.

   Drawing never goes to the window directly: ui.c draws into the offscreen
   port (gfx_begin()/gfx_end() bracket it), and an update event -- a desk
   accessory moved off the window, say -- is answered by copying the bitmap
   again rather than redrawing the game. */

#define GFX_W 496
#define GFX_H 296

void gfx_init(void);
/* Nonzero when drawing in colour (see macgfx.c); ui.c then uses RGB fills
   instead of the 1-bit suit patterns. */
unsigned char gfx_has_color(void);
void wait_vsync(void);

/* Make the offscreen port current for drawing; gfx_end() restores the
   port and marks rows top..bottom-1 to be copied to the window at the next
   wait_vsync(). Only changed rows are copied: a whole-window CopyBits on a
   Plus was caught mid-copy by a screenshot (a half-updated window), and
   copying less keeps that window short. */
void gfx_begin(void);
void gfx_end(short top, short bottom);

/* Input queued by the event loop. Keys are the event's character code;
   clicks are in window-local (= offscreen bitmap) coordinates. Both return
   0 when the queue is empty. */
int gfx_next_key(void);
int gfx_next_click(short *x, short *y);
/* Drop queued keys and clicks -- called when the human's turn starts, so a
   click made while a CPU player was thinking can't play a card. */
void gfx_flush_input(void);

/* Game > New Game. The handler is called from inside the event loop and is
   expected not to return: main.c longjmps back to the top of its game loop,
   abandoning whatever turn or prompt was in progress. */
void gfx_on_new_game(void (*handler)(void));

#endif
