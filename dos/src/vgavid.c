#include <stdio.h>
#include <dos.h>
#include <conio.h>
#include "gfx.h"
#include "cards.h"

/* VGA mode 13h. After the EGA backend next door -- Set/Reset registers, bit
   masks, latches loaded by dummy reads -- this file is almost embarrassing:
   a pixel is a byte, and setting it is a store. */

#define VRAM_SEG 0xA000
#define CRT_STATUS 0x3DA

/* DAC ports. Write the index to 0x3C8 (or 0x3C7 to read), then three
   6-bit components to 0x3C9; the index steps on automatically. */
#define DAC_READ_INDEX  0x3C7
#define DAC_WRITE_INDEX 0x3C8
#define DAC_DATA        0x3C9

static unsigned char far *vram = (unsigned char far *)MK_FP(VRAM_SEG, 0x0000);
static const unsigned char far *font;

/* The sixteen colours ui_bmp.c draws with, as 6-bit DAC triples. Only the
   suits and the felt really matter; the rest keep roughly their CGA
   meanings so the shared UI's incidental colours look sensible.

   This is what mode 13h buys over EGA: the four suits are the actual UNO
   colours rather than the nearest of a fixed sixteen, and the felt is a
   dark table green rather than a stock one. */
static const unsigned char palette[16][3] = {
    {  0,  0,  0 },   /* black       */
    {  0,  0, 42 },   /* blue        */
    {  0, 20,  8 },   /* felt green  */
    {  0, 42, 42 },   /* cyan        */
    { 40,  6,  6 },   /* card-back red */
    { 42,  0, 42 },   /* magenta     */
    { 42, 21,  0 },   /* brown       */
    { 42, 42, 42 },   /* light grey  */
    { 20, 20, 20 },   /* dark grey (wild) */
    { 12, 22, 60 },   /* UNO blue    */
    {  0, 50, 14 },   /* UNO green   */
    { 21, 63, 63 },   /* light cyan  */
    { 60, 12, 12 },   /* UNO red     */
    { 63, 21, 63 },   /* light magenta */
    { 63, 50,  0 },   /* UNO yellow  */
    { 63, 63, 63 }    /* white       */
};

void wait_vsync(void)
{
    while (inp(CRT_STATUS) & 0x08) { }
    while (!(inp(CRT_STATUS) & 0x08)) { }
}

void gfx_init(void)
{
    union REGS r;
    int i;

    r.x.ax = 0x0013;                 /* 320x200, 256 colours */
    int86(0x10, &r, &r);

    r.h.ah = 0x01;                   /* hide the cursor */
    r.x.cx = 0x2000;
    int86(0x10, &r, &r);

    /* Same trick the EGA backend uses: interrupt 43h holds a far pointer to
       the character generator for the current mode, 8x8 here. */
    font = *(const unsigned char far * far *)MK_FP(0x0000, 0x43 * 4);

    outp(DAC_WRITE_INDEX, 0);
    for (i = 0; i < 16; i++) {
        outp(DAC_DATA, palette[i][0]);
        outp(DAC_DATA, palette[i][1]);
        outp(DAC_DATA, palette[i][2]);
    }

    gfx_clear(GC_FELT);
}

void gfx_shutdown(void)
{
    union REGS r;
    r.x.ax = 0x0003;
    int86(0x10, &r, &r);
}

void gfx_fill_rect(int x, int y, int w, int h, unsigned char color)
{
    int row;

    if (w <= 0 || h <= 0) return;
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x >= GFX_W || y >= GFX_H || w <= 0 || h <= 0) return;
    if (x + w > GFX_W) w = GFX_W - x;
    if (y + h > GFX_H) h = GFX_H - y;

    for (row = 0; row < h; row++) {
        unsigned int off = (unsigned int)(y + row) * GFX_W + (unsigned int)x;
        int i;
        for (i = 0; i < w; i++) vram[off + i] = color;
    }
}

void gfx_clear(unsigned char color)
{
    gfx_fill_rect(0, 0, GFX_W, GFX_H, color);
}

void gfx_frame_rect(int x, int y, int w, int h, unsigned char color)
{
    gfx_fill_rect(x, y, w, 1, color);
    gfx_fill_rect(x, y + h - 1, w, 1, color);
    gfx_fill_rect(x, y, 1, h, color);
    gfx_fill_rect(x + w - 1, y, 1, h, color);
}

/* No byte-column alignment to worry about here, unlike EGA -- a glyph can
   start at any x. */
static void glyph(int x, int y, char ch, unsigned char color, int opaque,
                  unsigned char bg)
{
    const unsigned char far *gl = font + (unsigned int)(unsigned char)ch * GFX_FONT_H;
    int row, col;

    for (row = 0; row < GFX_FONT_H; row++) {
        unsigned char bits = gl[row];
        int yy = y + row;
        unsigned int off;
        if (yy < 0 || yy >= GFX_H) continue;
        off = (unsigned int)yy * GFX_W + (unsigned int)x;
        for (col = 0; col < GFX_FONT_W; col++) {
            int xx = x + col;
            if (xx < 0 || xx >= GFX_W) continue;
            if (bits & (0x80 >> col)) vram[off + col] = color;
            else if (opaque) vram[off + col] = bg;
        }
    }
}

void gfx_char(int x, int y, char ch, unsigned char color, unsigned char bg)
{
    glyph(x, y, ch, color, 1, bg);
}

void gfx_text(int x, int y, const char *s, unsigned char color, unsigned char bg)
{
    while (*s) {
        glyph(x, y, *s++, color, 1, bg);
        x += GFX_FONT_W;
    }
}

void gfx_text_t(int x, int y, const char *s, unsigned char color)
{
    while (*s) {
        glyph(x, y, *s++, color, 0, 0);
        x += GFX_FONT_W;
    }
}

int gfx_dump(const char *path)
{
    FILE *f = fopen(path, "wb");
    unsigned int row;
    int i;
    unsigned char line[GFX_W];

    if (f == NULL) return 1;

    fputc('V', f); fputc('G', f);
    fputc(GFX_W & 0xFF, f); fputc((GFX_W >> 8) & 0xFF, f);
    fputc(GFX_H & 0xFF, f); fputc((GFX_H >> 8) & 0xFF, f);

    /* The whole DAC, so the host renders exactly what was displayed rather
       than guessing at a palette. EGA cannot do this -- its palette
       registers are write-only -- which is why that backend's colours have
       to be mirrored in the host script instead. */
    outp(DAC_READ_INDEX, 0);
    for (i = 0; i < 768; i++) fputc(inp(DAC_DATA), f);

    for (row = 0; row < GFX_H; row++) {
        unsigned int base = row * GFX_W;
        for (i = 0; i < GFX_W; i++) line[i] = vram[base + i];
        fwrite(line, 1, GFX_W, f);
    }
    fclose(f);
    return 0;
}
