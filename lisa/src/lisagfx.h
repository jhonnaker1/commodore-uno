#ifndef LISAGFX_H
#define LISAGFX_H

/* The Lisa platform layer: the 720x364 1-bit screen, a few filled shapes
   and an embedded font to draw on it, the keyboard and mouse (through the
   COPS) with the mouse pointer, the frame clock.

   Every port's game loop is a busy loop that calls wait_vsync() once a
   frame. Here that function waits for the vertical retrace bit in the MMU
   status register, reading the keyboard while it waits, so no key is
   missed and nothing needs interrupts.

   Lisa pixels are about one and a half times as tall as they are wide, so
   anything meant to look round is drawn wider than it is tall: a card's
   corner is 9 pixels across and 6 down. */

#define GFX_W 720
#define GFX_H 364

/* Fill patterns, as a black-and-white Mac's: 8 rows of 8 pixels. */
#define PAT_WHITE 0
#define PAT_LTGRAY 1
#define PAT_GRAY 2
#define PAT_DKGRAY 3
#define PAT_BLACK 4

void gfx_init(void);
void wait_vsync(void);
/* Frames since start-up. */
unsigned long gfx_frames(void);

/* Keys pressed, oldest first; 0 when there are none. Letters come as
   lower case, digits as digits; Return '\r', Enter 0x03, space ' ', and
   the numeric keypad's 4/6/8/2 as the Mac's arrow codes 0x1C/0x1D/0x1E/
   0x1F (the Lisa keyboard has no arrow keys). */
int gfx_next_key(void);
/* Mouse clicks, oldest first: 1 and where the pointer was when the button
   went down, or 0 when there are none. */
int gfx_next_click(short *x, short *y);
/* Drop the keys and clicks queued so far: called when the human is asked something,
   so a key pressed while a CPU player was thinking can't answer it. */
void gfx_flush_input(void);

/* Shapes. x, y is the top left; w, h the size in pixels. */
void gfx_rect(short x, short y, short w, short h, unsigned char pat);
void gfx_invert(short x, short y, short w, short h);
/* Rounded rectangle, corners rx across and ry down. An oval is one whose
   corners are half its size. */
void gfx_round(short x, short y, short w, short h, short rx, short ry, unsigned char pat);
/* The same, filled in four quarters (top left, top right, bottom left,
   bottom right): a wild card. */
void gfx_round4(short x, short y, short w, short h, short rx, short ry,
                unsigned char tl, unsigned char tr, unsigned char bl, unsigned char br);
void gfx_round_invert(short x, short y, short w, short h, short rx, short ry);
/* A rounded frame t pixels thick at the sides and t*2/3 at top and bottom
   (so it looks even), with the inside left alone. */
void gfx_round_frame(short x, short y, short w, short h, short rx, short ry, short t);

/* Text with the 5x9 font, top left at x, y, each pixel drawn sx wide and
   sy tall. */
#define TXT_BOLD 1
#define TXT_WHITE 2 /* clear pixels instead of setting them */
void gfx_text(short x, short y, const char *s, unsigned char sx, unsigned char sy,
              unsigned char flags);
short gfx_text_w(const char *s, unsigned char sx, unsigned char flags);

#endif
