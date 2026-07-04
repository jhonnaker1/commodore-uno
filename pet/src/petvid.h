#ifndef PETVID_H
#define PETVID_H

/* Standard 40-column PET (4032 and siblings): screen matrix at $8000,
   40x25, stock character ROM only (no custom charset, no color RAM --
   the PET is monochrome). No accessible vsync signal, so pacing is done
   with the KERNAL jiffy clock instead of raster polling. */
#define SCREEN ((unsigned char *)0x8000)
#define COLS 40
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
