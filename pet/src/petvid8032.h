#ifndef PETVID8032_H
#define PETVID8032_H

/* PET 8032 (and other 80-column business PETs): same hardware family as
   the 4032 -- screen matrix still at $8000, monochrome, no color RAM --
   just 80x25 instead of 40x25 (the CRTC chip only changes timing/
   refresh, not the linear x+y*COLS addressing scheme). No accessible
   vsync signal, so pacing is done with the KERNAL jiffy clock instead of
   raster polling, same as the 4032. */
#define SCREEN ((unsigned char *)0x8000)
#define COLS 80
#define ROWS 25

void pet_init(void);
void wait_tick(void);
void scr_clear(void);
void scr_put(unsigned char x, unsigned char y, unsigned char ch);
void scr_puts(unsigned char x, unsigned char y, const char *s);
void scr_put_num(unsigned char x, unsigned char y, unsigned int n);
void scr_fill_rect(unsigned char x, unsigned char y, unsigned char w, unsigned char h,
                    unsigned char ch);

#endif
