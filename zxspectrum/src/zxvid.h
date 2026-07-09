#ifndef ZXVID_H
#define ZXVID_H

/* The Spectrum's screen is 32x24 text cells over a 256x192 bitmap, with
   colour applied per 8x8-pixel attribute cell (one ink + one paper colour
   per cell -- the famous "attribute clash"). There's no per-cell colour
   independent of the character glyph like the Commodore machines, and no
   cputcxy()-style direct API like cc65's conio.h either -- z88dk drives
   the screen through stdio with embedded control codes (crt1): \x10=INK,
   \x11=PAPER, \x14=INVERSE (swaps ink/paper), \x16=AT (position). Cards
   are shown as bracketed [label:COLORLETTER+VALUE] with a colour letter
   (same trick as the VIC-20/PET/Atari ports) since ink applies per whole
   character anyway, and selection uses INVERSE instead of a colour change. */
#define COLS 32
#define ROWS 24

void zx_init(void);
void wait_vsync(void);
void scr_clear(void);
void scr_put(unsigned char x, unsigned char y, char ch, unsigned char inverse);
void scr_puts(unsigned char x, unsigned char y, const char *s, unsigned char inverse);
void scr_put_num(unsigned char x, unsigned char y, unsigned int n);
void scr_fill_rect(unsigned char x, unsigned char y, unsigned char w, unsigned char h);

#endif
