#ifndef VIC_H
#define VIC_H

/* VIC bank 2 ($8000-$BFFF): screen matrix at $8400 (bank offset $400 --
   the C128 KERNAL's own IRQ periodically re-asserts $D018's screen-
   address bits back to that default, so we build to it rather than
   fight it every frame). Charset at $A000 (bank offset $2000): the
   KERNAL's own preferred char-base offset, $1800, is a hardware trap on
   VIC bank 0/2 (hardwired to show the internal character ROM instead of
   RAM), so unlike the screen address we do have to keep re-winning that
   part of the fight every frame -- see wait_vsync() in vic.c. cc65's
   default c128.cfg places program code/data in a contiguous region well
   below $8000, so there's no risk of code colliding with the display
   memory we poke directly. */
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

/* Sprite 0 is reserved for the "card toss" animation; sprite data lives at
   $8800, right after the screen's 1K block ($8400-$87FF) and well clear
   of the charset at $A000 -- unlike the C64 port's $9000 (right after its
   charset), C128's screen and charset aren't adjacent, so there's no
   single "right after the previous block" spot that works for both;
   $8800 was picked because it's the first free byte after the screen. */
void sprite_enable(unsigned char on);
void sprite_set_color(unsigned char color);
void sprite_set_pos(unsigned int x, unsigned char y);
unsigned int sprite_x_from_col(unsigned char col);
unsigned char sprite_y_from_row(unsigned char row);
void vic_set_border(unsigned char color);
void scr_clear(void);
void scr_put(unsigned char x, unsigned char y, unsigned char ch, unsigned char color);
void scr_puts(unsigned char x, unsigned char y, const char *s, unsigned char color);
void scr_put_num(unsigned char x, unsigned char y, unsigned int n, unsigned char color);
void scr_fill_rect(unsigned char x, unsigned char y, unsigned char w, unsigned char h,
                    unsigned char ch, unsigned char color);

#endif
