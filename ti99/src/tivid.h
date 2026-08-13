#ifndef TIVID_H
#define TIVID_H

/* TMS9918A video, Graphics I mode: 32x24 characters over a 256x192 screen.

   The TI's colour model is the odd one in this project. There is no colour
   RAM and no per-cell attribute byte -- the whole screen shares one 32-byte
   colour table, and each entry colours a *group of eight consecutive
   character codes*. Colour therefore belongs to the character code, not to
   the screen position: any two cells showing code 'R' are necessarily the
   same colour, everywhere, always.

   The classic TI answer, and the one used here, is to spend character codes
   to buy colours: keep several copies of the glyphs you need to draw in
   colour, each copy living in its own colour group. So this port carries six
   24-code ranges (the four suits, wild, and a highlight range for the
   cursor) -- three colour groups apiece -- each holding the same small
   alphabet of card glyphs, and picks which copy to draw by colour. The
   alphabet is 23 glyphs, so a range has one code to spare; 24 is what it
   takes to land on a group boundary, since a group is 8 codes and colour
   can only be assigned a whole group at a time. That buys solid colour tiles
   -- the same look as the X16/VBXE/F256 tile ports -- on a machine with no
   per-cell colour at all.

   The budget is exactly 32 groups (256 codes / 8), spent as:

     codes   0- 23  groups  0- 2   COL_SELECTED (cursor highlight)
     codes  24- 31  group   3      spare
     codes  32-127  groups  4-15   the console ASCII font, plain white text
     codes 128-151  groups 16-18   COL_RED
     codes 152-175  groups 19-21   COL_YELLOW
     codes 176-199  groups 22-24   COL_GREEN
     codes 200-223  groups 25-27   COL_BLUE
     codes 224-247  groups 28-30   COL_WILD
     codes 248-255  group  31      spare

   Only CGLYPHS (see tivid.c) exists in the coloured ranges -- 23 glyphs, not
   the full font -- which is why running text stays white and card faces are
   built from digits, suit letters and brackets. */

#define COLS 32
#define ROWS 24

/* Passed to scr_put/scr_puts to select which character-code range, and so
   which colour group, the glyph is drawn from. */
#define COL_NORMAL 0
#define COL_RED 1
#define COL_YELLOW 2
#define COL_GREEN 3
#define COL_BLUE 4
#define COL_WILD 5
#define COL_SELECTED 6

void ti_init(void);
void wait_vsync(void);
void scr_clear(void);
void scr_put(unsigned char x, unsigned char y, char ch, unsigned char color);
void scr_puts(unsigned char x, unsigned char y, const char *s, unsigned char color);
void scr_put_num(unsigned char x, unsigned char y, unsigned int n);
void scr_fill_rect(unsigned char x, unsigned char y, unsigned char w, unsigned char h);

#endif
