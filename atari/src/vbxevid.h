#ifndef VBXEVID_H
#define VBXEVID_H

/* VBXE (Video Board XE) is a hardware add-on for the Atari 8-bit line:
   a separate video chip with its own 512KB VRAM, shadowing the stock
   ANTIC/GTIA output with a much richer "Overlay" display. Its text
   mode is a real char+attribute scheme like the Commodore machines'
   color RAM -- 128 foreground and 128 background colors per character,
   picked from a 1024-color (21-bit) palette -- unlike stock Atari text
   mode, which shares one fg/bg for the whole screen (see atarivid.h).

   The narrowest of VBXE's three text widths is still 64 columns (its
   pixel modes go 256/320/336, but text mode doubles that to 512/640/
   672 for an "80-column" text capability, giving 64/80/84 characters
   -- there's no 40-column option), so this uses a wider layout than
   every other port here rather than reusing atari/src/ui.c's 40-col
   design. 24 rows x 8 scanlines = 192 scanlines, the full NTSC/PAL
   Atari display height. */
#define COLS 80
#define ROWS 24

#define COL_BLACK 0
#define COL_WHITE 1
#define COL_RED 2
#define COL_YELLOW 3
#define COL_GREEN 4
#define COL_BLUE 5
#define COL_CYAN 6
#define COL_MAGENTA 7
#define COL_ORANGE 8

/* "Tile" colors: each pairs a contrasting foreground with a SUIT-COLORED
   background (the b7=1 attribute format used throughout this driver ties
   every foreground index to its own independent background at index+128
   -- see make_attr() in vbxevid.c -- so a solid colored card face needs
   its own dedicated index, distinct from the plain-text COL_* indices
   above, which all pair with a black background). Used for the redefined-
   tile card rendering in ui_vbxe.c. */
#define TILE_RED 9
#define TILE_YELLOW 10
#define TILE_GREEN 11
#define TILE_BLUE 12
#define TILE_WILD 13
#define TILE_SELECTED 14
#define NUM_PALETTE_COLORS 15

void vbxe_init(void);
unsigned char vram_read_byte(unsigned long vram_addr);
void wait_vsync(void);
void scr_clear(void);
void scr_put(unsigned char x, unsigned char y, unsigned char ch, unsigned char color);
void scr_puts(unsigned char x, unsigned char y, const char *s, unsigned char color);
void scr_put_num(unsigned char x, unsigned char y, unsigned int n, unsigned char color);
void scr_fill_rect(unsigned char x, unsigned char y, unsigned char w, unsigned char h,
                    unsigned char ch, unsigned char color);

#endif
