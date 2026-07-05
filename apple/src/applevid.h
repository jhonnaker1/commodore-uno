#ifndef APPLEVID_H
#define APPLEVID_H

/* Standard Apple II 40-column text mode: 40 columns x 24 rows, already
   active at boot. Like the Atari, there is no per-character-cell color
   at all in text mode (monochrome), so cards use a color letter (same
   trick as VIC-20/PET/Atari) with reverse video for the selected card
   instead of a color change. */
#define COLS 40
#define ROWS 24

void apple_init(void);
void wait_vsync(void);
void scr_clear(void);
void scr_put(unsigned char x, unsigned char y, char ch, unsigned char inverse);
void scr_puts(unsigned char x, unsigned char y, const char *s, unsigned char inverse);
void scr_put_num(unsigned char x, unsigned char y, unsigned int n);
void scr_fill_rect(unsigned char x, unsigned char y, unsigned char w, unsigned char h);

#endif
