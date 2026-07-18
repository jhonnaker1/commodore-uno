#ifndef VBMP_H
#define VBMP_H

/* Bitmap graphics layer for the Commander X16, built on the KERNAL's
   GRAPH_* framebuffer API (320x240, 256 colours). See graph.s for the
   low-level thunks. Colours are 8-bit VERA palette indices; the default
   palette's low 16 match the C64 set (2=red, 5=green, 6=blue, 7=yellow,
   1=white, 0=black), and we reprogram a couple of high slots for the felt
   table and card shading. */

/* palette indices used by the card renderer */
#define GC_BLACK 0
#define GC_WHITE 1
#define GC_RED 2
#define GC_GREEN 5
#define GC_BLUE 6
#define GC_YELLOW 7
#define GC_LTGRAY 15
#define GC_FELT 16    /* reprogrammed: dark green table */
#define GC_SHADOW 17  /* reprogrammed: card drop shadow */

void gfx_init(void);
void gfx_set_colors(unsigned char stroke, unsigned char fill, unsigned char bg);
void gfx_clear(void);
void gfx_rect(unsigned int x, unsigned int y, unsigned int w, unsigned int h, unsigned char fill);
void gfx_oval(unsigned int x, unsigned int y, unsigned int w, unsigned int h, unsigned char fill);
void gfx_char(unsigned int x, unsigned int y, char c);
void gfx_text(unsigned int x, unsigned int y, const char *s);
/* reprogram one VERA palette entry (4-bit r,g,b) */
void gfx_palette(unsigned char idx, unsigned char r, unsigned char g, unsigned char b);

/* Draw an UNO card face at (x,y). suit: 0=red 1=yellow 2=green 3=blue,
   4=wild. value: 0-9 or a VAL_* action code. Draws the white card, suit
   border, corner values, and a centre pip with the value. */
void vb_card(unsigned int x, unsigned int y, unsigned char suit, unsigned char value);
/* Draw a face-down card back at (x,y). */
void vb_card_back(unsigned int x, unsigned int y);

/* Fan `count` cards (parallel suit/value arrays) starting at (hx,hy),
   overlapping so each shows its left corner strip; the `selected` card is
   lifted and highlighted. Spacing auto-tightens so large hands still fit. */
void vb_hand(unsigned int hx, unsigned int hy, const unsigned char *suits,
               const unsigned char *values, unsigned char count, unsigned char selected);

#endif
