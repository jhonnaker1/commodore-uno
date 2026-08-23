#ifndef M65NATIVE_H
#define M65NATIVE_H

/* MEGA65 *native mode* video layer: 80x25 text on the VIC-IV, via
   mega65-libc's conio built for llvm-mos.

   This is the counterpart to mega65vid.h, which drives the same machine
   from a cc65 c64-target binary. The difference is the load address: that
   build starts at $0801 and therefore runs in the MEGA65's C64 mode, where
   80 columns simply do not exist -- the C64 never had them, and setting
   VIC-IV's H640 bit from inside C64 mode doesn't change that. This build
   is linked to $2001 with a BASIC 65 header, so it runs in native mode and
   H640 is genuinely available.

   The second thing native mode buys is colour RAM. 80x25 is 2000 cells,
   which overflows the 1K $D800 window a C64-mode program is limited to;
   mega65-libc reaches the real colour RAM at $FF80000 through the 45GS02's
   32-bit addressing, so every one of those 2000 cells gets its own colour.

   The API is deliberately identical to mega65vid.h (and to the C128 port's
   vdc.h) so ui_native.c and mega65snd.c can be written against it without
   caring which backend is underneath. */

#define COLS 80
#define ROWS 25

/* VIC-IV palette indices -- the same 16 as the VIC-II, unlike the C128
   port's VDC, which is a separate RGBI chip needing its own mapping. */
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

/* OR this into a colour to draw a cell reverse-video, which paints the cell
   solid in that colour and knocks the glyph out of it in the background
   colour. That is how the card tiles get a value punched out of a solid
   suit block -- the same look the X16, VBXE, F256 and DOS ports build from
   per-cell foreground+background, done here with one attribute bit.
   Requires setextendedattrib(1), which mega65_init() does. */
#define COL_REVERSE 0x20

/* Stock PETSCII box-drawing characters, converted to screen codes by the
   driver. Same codes the 40-column build and the CBM-510 port use. */
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
