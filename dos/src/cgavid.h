#ifndef CGAVID_H
#define CGAVID_H

/* CGA text video: 80x25, two bytes per cell at B800:0000 -- a character and
   an attribute byte of (blink<<7) | (bg<<4) | fg.

   Per-cell foreground *and* background is exactly the model the X16, VBXE,
   F256 and MEGA65 tile ports use, so cards are drawn the same way here:
   solid blocks of the suit colour with the value knocked out in white.

   The catch, and the one interesting thing about CGA colour: the attribute
   byte only has three bits for background, so backgrounds are normally
   limited to the eight dark colours. Yellow would come out brown. Bit 7 is
   the blink flag, and turning blinking off re-purposes it as the background
   intensity bit -- all sixteen colours become available as backgrounds, and
   the suits can be the real UNO red/yellow/green/blue. cga_init() does
   that; see cgavid.c for how, since CGA and VGA want it done differently. */

#define COLS 80
#define ROWS 25

/* CGA's fixed 16-colour palette. Only 0-7 are available as backgrounds
   until blinking is disabled. */
#define C_BLACK     0
#define C_BLUE      1
#define C_GREEN     2
#define C_CYAN      3
#define C_RED       4
#define C_MAGENTA   5
#define C_BROWN     6
#define C_LGRAY     7
#define C_DGRAY     8
#define C_LBLUE     9
#define C_LGREEN   10
#define C_LCYAN    11
#define C_LRED     12
#define C_LMAGENTA 13
#define C_YELLOW   14
#define C_WHITE    15

#define ATTR(fg, bg) ((unsigned char)(((bg) << 4) | (fg)))

void cga_init(void);
void cga_shutdown(void);
/* Waits for the vertical retrace on the CGA status port. Present on VGA
   too, at the same address, so this works on both. */
void wait_vsync(void);

void scr_clear(unsigned char attr);
void scr_put(int x, int y, char ch, unsigned char attr);
void scr_puts(int x, int y, const char *s, unsigned char attr);
void scr_fill(int x, int y, int w, int h, char ch, unsigned char attr);
void scr_put_num(int x, int y, unsigned int n, unsigned char attr);

#endif
