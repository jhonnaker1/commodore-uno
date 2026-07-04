#ifndef VIC_H
#define VIC_H

/* VIC bank 2 ($8000-$BFFF): screen matrix at $8400, charset at $9800 --
   these specific offsets ($400 and $1800 into the bank) aren't a free
   choice. The C128 KERNAL's own IRQ periodically re-asserts $D018 back to
   its stock default ($17) no matter what vic_init() writes there, so
   rather than fight that every frame we just build to the addresses that
   value already implies. cc65's default c128.cfg places program code/data
   in a contiguous region well below $8000, so there's no risk of code
   colliding with the display memory we poke directly. */
#define SCREEN ((unsigned char *)0x8400)
#define COLOR ((unsigned char *)0xD800)

/* C64 colors */
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

void vic_init(void);
void wait_vsync(void);
void scr_clear(void);
void scr_put(unsigned char x, unsigned char y, unsigned char ch, unsigned char color);
void scr_puts(unsigned char x, unsigned char y, const char *s, unsigned char color);
void scr_put_num(unsigned char x, unsigned char y, unsigned int n, unsigned char color);
void scr_fill_rect(unsigned char x, unsigned char y, unsigned char w, unsigned char h,
                    unsigned char ch, unsigned char color);

#endif
