#ifndef VID510_H
#define VID510_H

/* Unlike every other Commodore port in this project, the CBM-II's video/
   color RAM and I/O chips live in a separate bank-switched "system bank"
   that plain pointer writes can't reach (cc65 provides pokebsys()/
   peekbsys() for that) -- confirmed the hard way (direct pokes at the
   documented $F000/$D400 addresses never showed up on screen, and
   probing bank 15 via VICE's monitor gave suspicious, inconsistent
   results). conio.h's clrscr()/cputcxy()/cputsxy()/textcolor() already
   handle that banking correctly internally, so this whole layer is just
   a thin wrapper matching every other port's scr_*() function shape, not
   a memory-mapped screen we poke directly.

   Deliberately NOT including <cbm.h> or <cbm510.h> here: <cbm.h>
   unconditionally pulls in <cbm510.h> for this target (even just for its
   CH_ULCORNER-style box-drawing constants), and <cbm510.h> has its own
   COLOR_RED/COLOR_YELLOW/COLOR_GREEN/COLOR_BLUE for the hardware palette
   that clash with cards.h's COLOR_RED/COLOR_YELLOW/COLOR_GREEN/COLOR_BLUE
   suit constants (same names, different numeric values) -- confirmed as
   a real "macro redefinition" build error, since ui.c includes both
   (transitively) at once. Every value below is hardcoded instead. */

#define COLS 40
#define ROWS 25

/* Real VIC-II + SID here (this is the one CBM-II model with both, unlike
   the rest of the CBM-II line) -- same 16-color palette as the C64,
   confirmed rendering distinct colors via textcolor(). */
#define COL_BLACK 0x00
#define COL_WHITE 0x01
#define COL_RED 0x02
#define COL_CYAN 0x03
#define COL_PURPLE 0x04
#define COL_GREEN 0x05
#define COL_BLUE 0x06
#define COL_YELLOW 0x07
#define COL_ORANGE 0x08
#define COL_LTGRAY 0x0F
#define COL_MDGRAY 0x0C
#define COL_DKGRAY 0x0B

/* Stock PETSCII box-drawing characters (same codes <cbm.h> defines as
   CH_ULCORNER etc) -- no custom charset here (unlike the C64/C128,
   there's no dumped chargen ROM for this rare a machine to build one
   from). */
#define CH_ULCORNER 176
#define CH_URCORNER 174
#define CH_LLCORNER 173
#define CH_LRCORNER 189
#define CH_HLINE 192
#define CH_VLINE 221

void cbm510_init(void);
void wait_vsync(void);
void scr_clear(void);
void scr_put(unsigned char x, unsigned char y, unsigned char ch, unsigned char color);
void scr_puts(unsigned char x, unsigned char y, const char *s, unsigned char color);
void scr_put_num(unsigned char x, unsigned char y, unsigned int n, unsigned char color);
void scr_fill_rect(unsigned char x, unsigned char y, unsigned char w, unsigned char h,
                    unsigned char ch, unsigned char color);
/* A solid color swatch, one character cell -- reverse-video space rather
   than a specific "solid block" character code, since the shifted-space
   block glyph (PETSCII 160) turned out not to render visibly on this
   machine's font (confirmed empirically: the card-back pattern and color
   swatches were blank). Reverse video just swaps foreground/background
   for the cell, which works regardless of what any particular glyph
   looks like. */
void scr_put_solid(unsigned char x, unsigned char y, unsigned char color);

#endif
