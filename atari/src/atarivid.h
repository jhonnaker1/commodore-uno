#ifndef ATARIVID_H
#define ATARIVID_H

/* Standard Atari 8-bit text mode (ANTIC mode 2, i.e. GRAPHICS 0) is
   already active at boot: 40 columns x 24 rows. Unlike the Commodore
   machines, this mode has no per-character-cell color -- the whole
   screen shares one foreground and one background color, so cards
   are distinguished with an explicit color letter (same trick used on
   the VIC-20 and PET ports) rather than a color change. Selection is
   shown with reverse video instead, which IS supported per character. */
#define COLS 40
#define ROWS 24

void atari_init(void);
void wait_vsync(void);
void scr_clear(void);
void scr_put(unsigned char x, unsigned char y, char ch, unsigned char inverse);
void scr_puts(unsigned char x, unsigned char y, const char *s, unsigned char inverse);
void scr_put_num(unsigned char x, unsigned char y, unsigned int n);
void scr_fill_rect(unsigned char x, unsigned char y, unsigned char w, unsigned char h);

#endif
