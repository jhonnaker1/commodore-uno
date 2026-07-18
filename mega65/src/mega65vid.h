#ifndef MEGA65VID_H
#define MEGA65VID_H

/* MEGA65 video layer, on mega65-libc's VIC-IV conio. 40x25 text with the
   MEGA65's per-cell colour RAM (and the reverse attribute for solid colour
   swatches), same card-box style as the CBM-510/C128 40-column ports.

   mega65-libc's cputcxy() writes raw SCREEN CODES, so this driver converts
   PETSCII -> screen code (so both text and the PETSCII box-drawing glyphs
   render), and sets each cell's colour precisely with cellcolor().

   The colour constants below are the real MEGA65 palette indices (which
   match the C64/VIC palette), so the shared 40-column ui.c uses them
   directly. */

#define COLS 40
#define ROWS 25

#define COL_BLACK 0
#define COL_WHITE 1
#define COL_RED 2
#define COL_CYAN 3
#define COL_PURPLE 4
#define COL_GREEN 5
#define COL_BLUE 6
#define COL_YELLOW 7
#define COL_ORANGE 8
#define COL_LTGRAY 15
#define COL_MDGRAY 12
#define COL_DKGRAY 11

/* Stock PETSCII box-drawing characters (converted to screen codes by the
   driver), same codes the CBM-510 port uses. */
#define CH_ULCORNER 176
#define CH_URCORNER 174
#define CH_LLCORNER 173
#define CH_LRCORNER 189
#define CH_HLINE 192
#define CH_VLINE 221

void mega65_init(void);
void wait_vsync(void);
void scr_clear(void);
void scr_put(unsigned char x, unsigned char y, unsigned char ch, unsigned char color);
void scr_puts(unsigned char x, unsigned char y, const char *s, unsigned char color);
void scr_put_num(unsigned char x, unsigned char y, unsigned int n, unsigned char color);
void scr_fill_rect(unsigned char x, unsigned char y, unsigned char w, unsigned char h,
                    unsigned char ch, unsigned char color);
/* A solid colour swatch (reverse-video space), one cell. */
void scr_put_solid(unsigned char x, unsigned char y, unsigned char color);

#endif
