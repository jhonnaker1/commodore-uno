#ifndef GFX_H_GUARD
#define GFX_H_GUARD

/* Common bitmap-graphics interface, implemented twice:

     egavid.c  EGA mode 10h  -- 640x350, 16 colours, four bit planes
     vgavid.c  VGA mode 13h  -- 320x200, 256 colours, one byte per pixel

   Those two are about as far apart as PC graphics hardware gets, which is
   the point of putting one interface over them: ui_bmp.c is written once
   against GFX_W/GFX_H and the drawing calls below, and lays itself out from
   those dimensions rather than from hardcoded coordinates. The backend
   header supplies the sizes and the colour names.

   Colours are indices. EGA uses its default 16-entry palette, whose bright
   red/yellow/green/blue are already the UNO colours; VGA reprograms its own
   because it can do better than the default 256. */

#if defined(GFX_EGA)
#include "egavid.h"
#elif defined(GFX_VGA)
#include "vgavid.h"
#else
#error "define GFX_EGA or GFX_VGA"
#endif

void gfx_init(void);
void gfx_shutdown(void);
void wait_vsync(void);

void gfx_clear(unsigned char color);
void gfx_fill_rect(int x, int y, int w, int h, unsigned char color);
void gfx_frame_rect(int x, int y, int w, int h, unsigned char color);
void gfx_char(int x, int y, char ch, unsigned char color, unsigned char bg);
void gfx_text(int x, int y, const char *s, unsigned char color, unsigned char bg);
/* Same, but with no background fill -- the glyph's set pixels only. Used
   over the felt, where a background box would show as a hard rectangle. */
void gfx_text_t(int x, int y, const char *s, unsigned char color);

/* Dump the framebuffer to a file for the host to check. Bitmap output can't
   be eyeballed as text the way the CGA build's screen dump can, so the
   smoke target writes the raw pixels and a host-side script turns them into
   a PNG. Returns 0 on success. */
int gfx_dump(const char *path);

#endif
