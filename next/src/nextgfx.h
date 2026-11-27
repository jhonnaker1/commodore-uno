#ifndef NEXTGFX_H
#define NEXTGFX_H

/* The NeXT platform layer (nextgfx.m): the application, its menu, the
   game window and a small set of drawing calls over Display PostScript.

   Every other port's game loop is a busy loop that calls wait_vsync() once
   a frame. Here that function is where the AppKit event loop lives: it
   flushes what was drawn to the screen, then takes events (keys, clicks,
   menu picks) until the next 1/60 s comes round. The game itself runs
   inside the application delegate's appDidInit:, so the shared loop never
   needs to know it is running under a window system.

   Coordinates are the Mac port's: a 496x296 area, origin top left, y
   down. The window shows it half as big again (744x444) by scaling the
   view, so the Mac layout and hit-testing carry over unchanged. */

#define GFX_W 496
#define GFX_H 296

void gfx_init(void);
/* Nonzero on a colour screen; on a greyscale NeXTstation the suits are
   four greys instead (ui.c). */
unsigned char gfx_has_color(void);
void wait_vsync(void);

/* Bracket drawing; what was drawn goes to the screen at the next
   wait_vsync(). */
void gfx_begin(void);
void gfx_end(void);

/* Input queued by the event loop. Keys are character codes, with the
   arrow keys as the Mac's 0x1C-0x1F and Enter as 0x03; clicks are in the
   496x296 coordinates. Both return 0 when the queue is empty. */
int gfx_next_key(void);
int gfx_next_click(short *x, short *y);
void gfx_flush_input(void);

/* The menu's New Game. The handler is called from inside the event loop
   and is expected not to return (main.c longjmps). */
void gfx_on_new_game(void (*handler)(void));

/* ---- drawing, between gfx_begin() and gfx_end() ----
   Rectangles are left, top, right, bottom, as QuickDraw's. */
void gfx_gray(float v);                 /* 0 black .. 1 white */
void gfx_rgb(float r, float g, float b);
void gfx_fill_rect(short l, short t, short r, short b);
void gfx_frame_rect(short l, short t, short r, short b);
void gfx_fill_round(short l, short t, short r, short b, float radius);
/* A frame w thick, inside the rectangle, as QuickDraw's pen. */
void gfx_frame_round(short l, short t, short r, short b, float radius, float w);
void gfx_fill_oval(short l, short t, short r, short b);
/* Limit drawing to a rounded rectangle until gfx_unclip(). */
void gfx_clip_round(short l, short t, short r, short b, float radius);
void gfx_unclip(void);

#define FONT_UI 0    /* Helvetica 12 */
#define FONT_BOLD 1  /* Helvetica-Bold 12 */
#define FONT_SMALL 2 /* Helvetica 9 */
#define FONT_BIG 3   /* Helvetica-Bold 24 */
void gfx_font(int which);
/* Text with its baseline at y, as QuickDraw's DrawText. */
void gfx_text(short x, short y, const char *s);
short gfx_text_w(const char *s);

#endif
