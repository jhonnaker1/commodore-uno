#ifndef VDC_H
#define VDC_H

/* Low-level VDC (8563/8568) indexed-register access. Every internal VDC
   register is reached through two I/O ports: write the register number to
   the control port, poll until ready, then read/write the data port. */
unsigned char vdc_reg_read(unsigned char reg);
void vdc_reg_write(unsigned char reg, unsigned char value);

/* Sets the VDC's internal "update address" (registers 18/19), used by
   vdc_data_write/read below for sequential access with auto-increment. */
void vdc_set_address(unsigned int addr);
void vdc_data_write(unsigned char value);
unsigned char vdc_data_read(void);

/* For bulk sequential transfers: select register 31 once, then hammer the
   data port directly (still ready-gated) without reselecting each byte. */
void vdc_reg_select_31(void);
void vdc_data_write_selected(unsigned char value);
unsigned char vdc_data_read_selected(void);

/* High-level 80x25 attribute-color text screen, layered on the above. */
#define VDC_COLS 80
#define VDC_ROWS 25
#define VDC_SCREEN_BASE 0x0000
#define VDC_ATTR_BASE 0x0800
#define VDC_CHARSET_BASE 0x2000

/* These were copy-pasted from vic.h's VIC-II palette without checking
   whether the VDC (8563/8568) actually shares it -- it doesn't. The VDC
   is an entirely separate RGBI-based color chip (bits 3/2/1/0 = Red/
   Green/Blue/Intensity, not VIC-II's palette order), with its own 16-
   entry table: 0=black, 1=dark gray, 2=dark blue, 3=light blue,
   4=dark green, 5=light green, 6=dark cyan, 7=light cyan, 8=dark red,
   9=light red, 10=dark purple, 11=light purple, 12=dark yellow,
   13=light yellow, 14=light gray, 15=white. Under the old (wrong)
   values, COL_RED (2) actually rendered as dark blue and COL_YELLOW (7)
   as light cyan -- exactly the "only shades of blue and green, no red
   or yellow" symptom reported after the sprite/animation parity work,
   though this mapping was wrong well before that; it just hadn't been
   looked at closely until then. Picking the "light" (brighter) variant
   of each hue throughout for the same reason VIC-II's own palette
   favors vivid primaries: readability. COL_ORANGE/MDGRAY/DKGRAY aren't
   used anywhere in ui_vdc.c/main_vdc.c today; the RGBI table only has
   two real gray steps (1, 14) rather than VIC-II's three, so these are
   reasonable placeholders rather than verified-correct choices. */
#define COL_BLACK 0
#define COL_WHITE 15
#define COL_RED 9
#define COL_CYAN 7
#define COL_PURPLE 11
#define COL_GREEN 5
#define COL_BLUE 3
#define COL_YELLOW 13
#define COL_ORANGE 12
#define COL_LTGRAY 14
#define COL_MDGRAY 1
#define COL_DKGRAY 1

void vdc_init(void);
void wait_vsync(void);
void scr_clear(void);
void scr_put(unsigned char x, unsigned char y, unsigned char ch, unsigned char color);
void scr_puts(unsigned char x, unsigned char y, const char *s, unsigned char color);
void scr_put_num(unsigned char x, unsigned char y, unsigned int n, unsigned char color);
void scr_fill_rect(unsigned char x, unsigned char y, unsigned char w, unsigned char h,
                    unsigned char ch, unsigned char color);

#endif
