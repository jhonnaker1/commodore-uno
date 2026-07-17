#ifndef X16VERA_H
#define X16VERA_H

/* Commander X16 video driver, written straight against VERA (the X16's
   video chip) rather than through cc65's conio. VERA's text mode gives a
   real char + per-cell color attribute (independent 4-bit foreground AND
   background palette index per cell), so -- like the Atari VBXE port and
   unlike a plain color-letter port -- cards render as solid colored TILES
   with a contrasting value character on top. The logical color set and
   the scr_*() API below are deliberately identical in shape to the VBXE
   driver so the UI layer ports across almost verbatim.

   Text map lives in VRAM at $1B000 (the KERNAL's default), 128 cells wide
   (only the first 80 are visible), each cell = 2 bytes: a screen-code and
   a color byte (background<<4 | foreground). We write it through VERA's
   auto-incrementing data port. See x16vera.c for the register details. */

#define COLS 80
#define ROWS 60

/* Logical colors -- same indices/order as the VBXE driver. 0-8 are plain
   text colors (foreground on a black cell); 9-14 are solid card TILES
   (suit-colored background, contrasting value char); 15-20 are the dimmed
   "not legal to play" tiles and the pulsing selection-bar shades. Each
   maps to a concrete VERA color byte in x16vera.c's cell_color[] table. */
#define COL_BLACK 0
#define COL_WHITE 1
#define COL_RED 2
#define COL_YELLOW 3
#define COL_GREEN 4
#define COL_BLUE 5
#define COL_CYAN 6
#define COL_MAGENTA 7
#define COL_ORANGE 8
#define TILE_RED 9
#define TILE_YELLOW 10
#define TILE_GREEN 11
#define TILE_BLUE 12
#define TILE_WILD 13
#define TILE_SELECTED 14
#define TILE_RED_DIM 15
#define TILE_YELLOW_DIM 16
#define TILE_GREEN_DIM 17
#define TILE_BLUE_DIM 18
#define TILE_WILD_DIM 19
#define TILE_SEL_DIM 20
#define NUM_LOGICAL 21

void vera_init(void);
void wait_vsync(void);
void scr_clear(void);
void scr_put(unsigned char x, unsigned char y, unsigned char ch, unsigned char color);
void scr_puts(unsigned char x, unsigned char y, const char *s, unsigned char color);
void scr_put_num(unsigned char x, unsigned char y, unsigned int n, unsigned char color);
void scr_fill_rect(unsigned char x, unsigned char y, unsigned char w, unsigned char h,
                    unsigned char ch, unsigned char color);

/* VERA hardware sprite used for the card-toss animation (Tier 3): a real
   moving sprite, not a redraw-based block. spr_show sets it to a suit
   color and position and enables it; spr_move glides it; spr_hide turns
   it off. Coordinates are in character cells (converted to pixels inside).
   `suit` is a COLOR_RED..COLOR_BLUE index (0-3) or COLOR_WILD. */
void spr_init(void);
void spr_show(unsigned char suit, unsigned char cx, unsigned char cy);
void spr_move(unsigned char cx, unsigned char cy);
void spr_hide(void);

#endif
