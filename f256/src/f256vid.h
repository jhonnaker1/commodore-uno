#ifndef F256VID_H
#define F256VID_H

/* Foenix F256 video: Vicky text mode. Like the X16 (VERA) and Atari VBXE
   ports, the F256 has a real per-cell foreground+background colour text
   mode, so UNO cards render as solid colour tiles with a contrasting value
   character -- no bitmap or custom charset needed.

   The screen is TWO matrices, both mapped at $C000 through the 8K I/O
   window that MMU_IO_CTRL ($0001) pages: char codes when $0001 == 2, and
   colour bytes when $0001 == 3. The Vicky control registers (master
   control, palettes, border) live at $D000+ in I/O page 0. A colour-matrix
   byte is (foreground<<4) | background, each nibble indexing a 16-entry
   CLUT. */

#define F256_COLS 80
#define F256_ROWS 60

/* logical colour indices (programmed into the CLUTs by f256_palette) */
#define FC_BLACK 0
#define FC_WHITE 1
#define FC_RED 2
#define FC_GREEN 3
#define FC_BLUE 4
#define FC_YELLOW 5
#define FC_GRAY 6
#define FC_FELT 7
#define FC_SHADOW 8

#define F256_COLOR(fg, bg) (unsigned char)(((fg) << 4) | (bg))

void f256_text_init(void);
/* ~one-frame busy delay, used to pace "thinking" pauses (no vblank wait). */
void wait_vsync(void);
/* set CLUT entry idx (0-15) to r,g,b (0-255); sets both the fg and bg LUTs */
void f256_palette(unsigned char idx, unsigned char r, unsigned char g, unsigned char b);
void f256_clear(unsigned char color);
void f256_put(unsigned char x, unsigned char y, unsigned char ch, unsigned char color);
void f256_puts(unsigned char x, unsigned char y, const char *s, unsigned char color);
void f256_fill(unsigned char x, unsigned char y, unsigned char w, unsigned char h,
               unsigned char ch, unsigned char color);

#endif
