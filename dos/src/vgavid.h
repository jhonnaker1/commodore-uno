#ifndef VGAVID_H
#define VGAVID_H

/* VGA mode 13h: 320x200, 256 colours, one byte per pixel.

   The opposite trade from the EGA backend, which shares this interface.
   EGA has nearly four times the pixels; mode 13h has a *linear chunky*
   framebuffer -- `screen[y*320+x] = colour`, no planes, no latches, no
   Graphics Controller registers, no bit masks. Setting one pixel is one
   store. It is the simplest video hardware in this entire repo, easier
   than the C64's, and it arrived last.

   The other thing it has is a real palette: 256 entries chosen from
   262,144, so unlike EGA the suits are not picked from a fixed sixteen but
   programmed to the actual UNO colours. */

#define GFX_W 320
#define GFX_H 200

/* Indices 0-15 keep the EGA/CGA names so ui_bmp.c can be shared verbatim,
   but the values behind them are programmed in gfx_init(). */
#define GC_BLACK    0
#define GC_BLUE     1
#define GC_GREEN    2
#define GC_CYAN     3
#define GC_RED      4
#define GC_MAGENTA  5
#define GC_BROWN    6
#define GC_LGRAY    7
#define GC_DGRAY    8
#define GC_LBLUE    9
#define GC_LGREEN  10
#define GC_LCYAN   11
#define GC_LRED    12
#define GC_LMAGENTA 13
#define GC_YELLOW  14
#define GC_WHITE   15

#define GC_FELT    GC_GREEN
#define GC_SHADOW  GC_BLACK

/* Half the EGA card in each axis, for half the screen in each axis. */
#define GFX_CARD_W 28
#define GFX_CARD_H 34

/* Mode 13h's ROM font is 8x8. */
#define GFX_FONT_W 8
#define GFX_FONT_H 8

#endif
