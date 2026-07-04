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

void vdc_init(void);
void wait_vsync(void);
void scr_clear(void);
void scr_put(unsigned char x, unsigned char y, unsigned char ch, unsigned char color);
void scr_puts(unsigned char x, unsigned char y, const char *s, unsigned char color);
void scr_put_num(unsigned char x, unsigned char y, unsigned int n, unsigned char color);
void scr_fill_rect(unsigned char x, unsigned char y, unsigned char w, unsigned char h,
                    unsigned char ch, unsigned char color);

#endif
