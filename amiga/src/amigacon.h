#ifndef AMIGACON_H
#define AMIGACON_H

/* Unlike every other platform in this project, the Amiga has no simple
   memory-mapped text screen to poke -- text is drawn through a CON:
   window (an AmigaDOS console handle backed by console.device), using
   documented ANSI/AmigaDOS escape sequences for cursor positioning and
   color rather than direct video RAM writes. This gives real per-cell
   color "for free" without any custom character set or hardware
   register work, unlike the 8-bit ports. */

#define COLS 80
#define ROWS 25

/* Standard ANSI 8-color palette (CSI 3<n> m), same numbering the
   AmigaDOS console recognizes. */
#define COL_BLACK 0
#define COL_RED 1
#define COL_GREEN 2
#define COL_YELLOW 3
#define COL_BLUE 4
#define COL_MAGENTA 5
#define COL_CYAN 6
#define COL_WHITE 7
#define COL_LTGRAY 7
#define COL_MDGRAY 7
#define COL_DKGRAY 7
#define COL_ORANGE 3
#define COL_PURPLE 5

void amiga_init(void);
void amiga_shutdown(void);
void wait_vsync(void);
void scr_clear(void);
void scr_put(unsigned char x, unsigned char y, char ch, unsigned char color);
void scr_puts(unsigned char x, unsigned char y, const char *s, unsigned char color);
void scr_put_num(unsigned char x, unsigned char y, unsigned int n, unsigned char color);
void scr_fill_rect(unsigned char x, unsigned char y, unsigned char w, unsigned char h,
                    unsigned char ch, unsigned char color);

/* Non-blocking: returns the next raw byte from the console if one is
   waiting (within a ~1/50s poll), else -1. */
int con_getkey(void);

#endif
