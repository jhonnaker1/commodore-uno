#ifndef MSXVDP_H
#define MSXVDP_H

/* V9938 (MSX2 VDP) graphics layer, SCREEN 5: 256x212, 16 colours chosen
   from a programmable 512-colour palette.

   Two things here are unavailable to the MSX1/TI-99 TMS9918A ports and
   shape the whole design:

     - the palette is programmable, so the four UNO suit colours are the
       actual UNO colours rather than the nearest of fifteen fixed ones; and
     - there is a hardware blitter, so filled rectangles cost the Z80 a
       15-byte command block instead of a write per pixel. Card bodies,
       borders and screen clears all go through it, which is what makes
       real pixel-art cards affordable at 3.58MHz.

   Text is the one thing the blitter cannot help with, so glyphs are still
   expanded and pushed by the CPU -- but from the machine's own ROM font
   rather than a copy carried in the cartridge. */

#define GFX_W 256
#define GFX_H 212

/* Palette indices, programmed in gfx_init(). Index 0 is left alone: in
   SCREEN 5 it is the transparent colour, and showing the backdrop through
   it is not what any of this wants. */
#define GC_WHITE  1
#define GC_RED    2
#define GC_YELLOW 3
#define GC_GREEN  4
#define GC_BLUE   5
#define GC_GREY   6
#define GC_SHADOW 7
#define GC_FELT   8
#define GC_BLACK  15

/* Card face size, used by the UI for layout. */
#define GFX_CARD_W 28
#define GFX_CARD_H 40

void gfx_init(void);
void wait_vsync(void);

void gfx_clear(unsigned char color);
/* x and w are rounded down to even: one byte holds two pixels in SCREEN 5
   and the blitter fills whole bytes. */
void gfx_fill_rect(int x, int y, int w, int h, unsigned char color);
void gfx_frame_rect(int x, int y, int w, int h, unsigned char color);

/* x must be even (see above). Both take an explicit background colour --
   glyphs are drawn as solid 8x8 blocks, so the caller says what the paper
   is: felt for messages, white for the labels on a card body. */
void gfx_char(int x, int y, char ch, unsigned char fg, unsigned char bg);
void gfx_text(int x, int y, const char *s, unsigned char fg, unsigned char bg);

/* Draw an UNO card face at (x,y). suit 0=red 1=yellow 2=green 3=blue
   4=wild; value 0-9 or a VAL_* action code. */
void gfx_card(int x, int y, unsigned char suit, unsigned char value);
void gfx_card_back(int x, int y);
/* Fan `count` cards from (hx,hy), overlapping so each shows its left edge;
   the selected card lifts clear of the fan and gets a highlight frame. */
void gfx_hand(int hx, int hy, const unsigned char *suits,
              const unsigned char *values, unsigned char count,
              unsigned char selected);

#endif
