#ifndef MEGA65SPR_H
#define MEGA65SPR_H

/* A single VIC-II hardware sprite (sprite 0) used for the card-toss
   animation. The MEGA65 in C64 mode keeps the legacy sprite path: with
   mega65-libc's screen at $0400 (VIC bank 0), the sprite pointer sits at
   $07F8 and sprite data goes in bank 0 (the tape buffer at $0340). So this
   is the same approach as the C64 port. Colors and cell coordinates match
   the video layer. */

void spr_init(void);
/* Show the card sprite in `color` (a COL_* palette index) at character
   cell (col,row); enables it. */
void spr_show(unsigned char color, unsigned char col, unsigned char row);
/* Move the (already shown) sprite to cell (col,row). */
void spr_move(unsigned char col, unsigned char row);
void spr_hide(void);

#endif
