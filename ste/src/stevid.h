#ifndef STEVID_H
#define STEVID_H

/* Atari ST/STE video layer: ST low resolution -- 320x200, 16 colours.

   Unlike every other machine in this repo, the ST has no text mode at all,
   and its framebuffer is not chunky: it is FOUR BITPLANES INTERLEAVED BY
   WORD. Each group of 8 bytes holds 16 pixels, as four 16-bit words (one
   per plane); a pixel's colour index is assembled from the same bit position
   in those four words. So plotting is read-modify-write on 4 words, and the
   fast path for solid fills is to write whole words with a bit mask rather
   than to touch pixels individually (see ste_fill_rect).

   Colour: the palette is programmed with STE 4-bit-per-gun values. The STE
   stores that 4th bit in bit 3 of each nibble as the LEAST significant bit,
   which a plain ST simply ignores -- so the same palette words give 4096
   colours on an STE and degrade to the nearest of the ST's 512 automatically,
   with no separate code path. */

#define BMP_W 320
#define BMP_H 200
#define BMP_STRIDE 160          /* bytes per scanline (320px / 16 * 8) */

/* palette indices used by the card renderer + UI */
#define SC_BLACK 0
#define SC_WHITE 1
#define SC_RED 2
#define SC_GREEN 3
#define SC_BLUE 4
#define SC_YELLOW 5
#define SC_GRAY 6
#define SC_SHADOW 7
#define SC_FELT 8

/* card face dimensions (used by the UI for layout) */
#define STE_CARD_W 34
#define STE_CARD_H 48

void ste_init(void);
void ste_shutdown(void);
/* 1 if an STE (or Mega STE) was detected, 0 for a plain ST */
int ste_is_ste(void);
/* set palette entry idx (0-15) from 4-bit r,g,b (0-15) */
void ste_palette(unsigned char idx, unsigned char r, unsigned char g, unsigned char b);
void ste_wait_vsync(void);

void ste_clear(unsigned char color);
void ste_pixel(unsigned int x, unsigned int y, unsigned char color);
void ste_hline(unsigned int x, unsigned int y, unsigned int w, unsigned char color);
void ste_fill_rect(unsigned int x, unsigned int y, unsigned int w, unsigned int h, unsigned char color);
void ste_frame_rect(unsigned int x, unsigned int y, unsigned int w, unsigned int h, unsigned char color);
void ste_char(unsigned int x, unsigned int y, unsigned char ch, unsigned char fg);
void ste_text(unsigned int x, unsigned int y, const char *s, unsigned char fg);

/* Draw an UNO card face at (x,y). suit 0=red 1=yellow 2=green 3=blue 4=wild;
   value 0-9 or a VAL_* action code. */
void ste_card(unsigned int x, unsigned int y, unsigned char suit, unsigned char value);
/* Draw a face-down card back at (x,y). */
void ste_card_back(unsigned int x, unsigned int y);
/* Fan `count` cards (parallel suit/value arrays) starting at (hx,hy),
   overlapping so each shows its left corner strip; the `selected` card is
   lifted and highlighted. Spacing auto-tightens so large hands still fit. */
void ste_hand(unsigned int hx, unsigned int hy, const unsigned char *suits,
              const unsigned char *values, unsigned char count, unsigned char selected);

#endif
