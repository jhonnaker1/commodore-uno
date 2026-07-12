#ifndef ZXVID_H
#define ZXVID_H

/* The Spectrum's screen is 32x24 text cells over a 256x192 bitmap, with
   colour applied per 8x8-pixel attribute cell (one ink + one paper colour
   per cell -- the famous "attribute clash"). That clash is a sub-cell
   bitmap-graphics problem, though: in plain text, one character IS one
   attribute cell, so ink can still be set per character with no clash at
   all -- confirmed by z88dk's crt1 console driver, which drives the
   screen through stdio with embedded control codes: \x10=INK, \x11=PAPER,
   \x14=INVERSE (swaps ink/paper), \x16=AT (position). Cards are still
   shown as bracketed [label:COLORLETTER+VALUE] like the VIC-20/PET/Atari
   ports (no redefinable character generator here either, so no custom
   card-shaped tiles like the C64 port), but the bracket/letter/value are
   now drawn in the card's real colour via \x10, not a single ink -- same
   upgrade as the CoCo 3 port's real per-suit colour.

   Raw ink values (0-7, z88dk's INK_* order): 0=black 1=blue 2=red
   3=magenta 4=green 5=cyan 6=yellow 7=white. Background is PAPER_BLACK
   (was PAPER_WHITE) specifically so yellow ink -- one of the four suit
   colours -- stays readable; yellow-on-white is notoriously low-contrast,
   a well-known Spectrum "attribute clash" era complaint, and every ink
   colour reads cleanly against black. */
#define COLS 32
#define ROWS 24

#define COL_RED 2
#define COL_YELLOW 6
#define COL_GREEN 4
#define COL_BLUE 1
#define COL_NORMAL 7
#define COL_WILD 3
#define COL_SELECTED 5

void zx_init(void);
void wait_vsync(void);
void scr_clear(void);
void scr_put(unsigned char x, unsigned char y, char ch, unsigned char color);
void scr_puts(unsigned char x, unsigned char y, const char *s, unsigned char color);
void scr_put_num(unsigned char x, unsigned char y, unsigned int n);
void scr_fill_rect(unsigned char x, unsigned char y, unsigned char w, unsigned char h);

#endif
