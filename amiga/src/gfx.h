#ifndef GFX_H_GUARD
#define GFX_H_GUARD

/* Bitmap-graphics layer for the Amiga UNO build. Where the text build
   (amigacon.c) draws colored characters through console.device, this owns a
   custom 320x256 16-colour lores screen and draws pixel-art cards into it
   with graphics.library primitives -- RectFill (blitter-accelerated) for the
   card bodies and the topaz ROM font via Text() for labels. The Amiga's
   planar framebuffer is handled by the OS here, so unlike the Atari ST port
   there is no hand-rolled bitplane masking. */

#define GFX_W 320
#define GFX_H 256

/* palette indices (programmed in gfx_init) */
#define GC_BLACK 0
#define GC_WHITE 1
#define GC_RED 2
#define GC_GREEN 3
#define GC_BLUE 4
#define GC_YELLOW 5
#define GC_GRAY 6
#define GC_SHADOW 7
#define GC_FELT 8

/* card face dimensions (used by the UI for layout) */
#define GFX_CARD_W 34
#define GFX_CARD_H 48

void gfx_init(void);
void gfx_shutdown(void);
void wait_vsync(void);

void gfx_clear(unsigned char color);
void gfx_fill_rect(int x, int y, int w, int h, unsigned char color);
void gfx_frame_rect(int x, int y, int w, int h, unsigned char color);
void gfx_char(int x, int y, char ch, unsigned char color);
void gfx_text(int x, int y, const char *s, unsigned char color);

/* Draw an UNO card face at (x,y). suit 0=red 1=yellow 2=green 3=blue 4=wild;
   value 0-9 or a VAL_* action code. */
void gfx_card(int x, int y, unsigned char suit, unsigned char value);
/* Draw a face-down card back at (x,y). */
void gfx_card_back(int x, int y);
/* Fan `count` cards (parallel suit/value arrays) from (hx,hy), overlapping
   so each shows its left corner strip; the `selected` card lifts + highlights. */
void gfx_hand(int hx, int hy, const unsigned char *suits,
              const unsigned char *values, unsigned char count, unsigned char selected);

#endif
