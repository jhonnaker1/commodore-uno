#ifndef VBXEBMP_H
#define VBXEBMP_H

/* Atari VBXE bitmap-graphics driver: brings up the VBXE Overlay in SR
   (standard-resolution) mode -- a 320x192, 8-bit-per-pixel chunky
   framebuffer in VBXE VRAM with a 256-entry, 24-bit palette. Unlike the
   text overlay (vbxevid.c), this is a real linear framebuffer with no
   attribute constraints, so cards can be drawn as full-color pixel art.

   The overlay mode is selected by the VBXE XDL: SR bitmap needs the GMON
   control bit (xdl1 & 3 == 2) rather than text's TMON, per Altirra's
   vbxe.cpp kOvModeTable. Pixels are 8-bit palette indices; the framebuffer
   is reached from the CPU through the 8K MEMAC window (banked, since it
   spans more than 64K of address space, though it all fits under $10000 of
   VBXE VRAM -- which lets the drawing hot paths use fast 16-bit math). */

#define BMP_W 320
#define BMP_H 192

/* palette indices used by the card renderer + UI (programmed in ui_bmp.c) */
#define VC_BLACK 0
#define VC_WHITE 1
#define VC_RED 2
#define VC_GREEN 3
#define VC_BLUE 4
#define VC_YELLOW 5
#define VC_GRAY 12
#define VC_SHADOW 15
#define VC_FELT 16

/* card face dimensions (used by the UI for layout) */
#define VBMP_CARD_W 34
#define VBMP_CARD_H 48

void vbmp_init(void);
void vbmp_wait_vsync(void);
/* set palette entry idx (0-255) to 8-bit r,g,b */
void vbmp_palette(unsigned char idx, unsigned char r, unsigned char g, unsigned char b);
void vbmp_clear(unsigned char color);
void vbmp_pixel(unsigned int x, unsigned char y, unsigned char color);
void vbmp_hline(unsigned int x, unsigned char y, unsigned int w, unsigned char color);
void vbmp_fill_rect(unsigned int x, unsigned char y, unsigned int w, unsigned char h, unsigned char color);
void vbmp_frame_rect(unsigned int x, unsigned char y, unsigned int w, unsigned char h, unsigned char color);
/* blit an 8x8 ROM-font glyph (ASCII ch) at (x,y) in colour fg */
void vbmp_char(unsigned int x, unsigned char y, unsigned char ch, unsigned char fg);
void vbmp_text(unsigned int x, unsigned char y, const char *s, unsigned char fg);

/* Draw an UNO card face at (x,y). suit 0=red 1=yellow 2=green 3=blue 4=wild;
   value 0-9 or a VAL_* action code. */
void vbmp_card(unsigned int x, unsigned char y, unsigned char suit, unsigned char value);
/* Draw a face-down card back at (x,y). */
void vbmp_card_back(unsigned int x, unsigned char y);
/* Fan `count` cards (parallel suit/value arrays) starting at (hx,hy),
   overlapping so each shows its left corner strip; the `selected` card is
   lifted and highlighted. Spacing auto-tightens so large hands still fit. */
void vbmp_hand(unsigned int hx, unsigned char hy, const unsigned char *suits,
               const unsigned char *values, unsigned char count, unsigned char selected);

#endif
