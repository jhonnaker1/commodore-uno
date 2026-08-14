#ifndef EGAVID_H
#define EGAVID_H

/* EGA mode 10h: 640x350, 16 colours, four bit planes.

   The highest resolution of any port in this repo -- more than four times
   the pixels of the Atari ST and Amiga builds, and nearly four times the
   VGA mode 13h one that shares this interface.

   The colours are the EGA default palette, not a reprogrammed one. EGA can
   pick its 16 from 64, but the default set already contains bright red,
   yellow, green and blue at the same indices CGA uses, which is exactly
   what the suits need -- so the port spends its effort on resolution
   instead. (The VGA backend does reprogram, because there the default 256
   are not what anyone wants.) */

#define GFX_W 640
#define GFX_H 350

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

/* Named roles the shared UI draws with. */
#define GC_FELT    GC_GREEN
#define GC_SHADOW  GC_BLACK

/* Card face size. Ten across the 640-pixel screen at a 64-pixel step. */
#define GFX_CARD_W 56
#define GFX_CARD_H 70

/* The ROM font this mode uses is 8x14. */
#define GFX_FONT_W 8
#define GFX_FONT_H 14

#endif
