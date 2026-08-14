#include <stdio.h>
#include <dos.h>
#include <conio.h>
#include "gfx.h"
#include "cards.h"

/* EGA mode 10h. Four bit planes all mapped at the same address, which is
   the thing that makes EGA unlike every other bitmap port here.

   The Amiga and Atari ST are also planar, but there the planes are ordinary
   CPU memory the program addresses itself. EGA's are not: all four sit
   behind A000:0000 at once, and which of them a write reaches -- and what
   value it writes -- is decided by registers in the Graphics Controller,
   not by the address. A CPU write does not carry data to the card so much
   as trigger the card's own ALU.

   The mechanism used throughout here is Set/Reset. With the Set/Reset
   register holding a colour and Enable Set/Reset turned on for all four
   planes, the byte the CPU writes is ignored as data: each plane instead
   takes the corresponding bit of the colour. The Bit Mask register selects
   which of the eight pixels in the byte are affected; the rest keep what
   the latches hold, which is why the byte must be read first (a read loads
   all four latches).

   One byte-write therefore paints up to eight pixels a solid colour across
   all four planes at once -- which suits card tiles exactly. */

#define VRAM_SEG 0xA000
#define ROW_BYTES (GFX_W / 8)

/* Graphics Controller: index port, data port, and the indices used. */
#define GC_INDEX 0x3CE
#define GC_DATA  0x3CF
#define GC_SET_RESET        0
#define GC_ENABLE_SET_RESET 1
#define GC_DATA_ROTATE      3
#define GC_READ_MAP         4
#define GC_MODE             5
#define GC_BIT_MASK         8

/* Sequencer: the Map Mask decides which planes a write can reach at all. */
#define SEQ_INDEX 0x3C4
#define SEQ_DATA  0x3C5
#define SEQ_MAP_MASK 2

#define CRT_STATUS 0x3DA

static unsigned char far *vram = (unsigned char far *)MK_FP(VRAM_SEG, 0x0000);
static const unsigned char far *font;

static void gc(unsigned char index, unsigned char value)
{
    outp(GC_INDEX, index);
    outp(GC_DATA, value);
}

static void seq(unsigned char index, unsigned char value)
{
    outp(SEQ_INDEX, index);
    outp(SEQ_DATA, value);
}

/* Point the card's ALU at a colour, for the run of writes that follows. */
static void set_color(unsigned char color)
{
    gc(GC_SET_RESET, (unsigned char)(color & 0x0F));
    gc(GC_ENABLE_SET_RESET, 0x0F);
}

/* Paint `mask`'s bits of one byte-column with the current Set/Reset colour.
   The read is not for its value -- it is what loads the latches, so the
   bits outside the mask survive. */
static void poke_masked(unsigned int off, unsigned char mask)
{
    volatile unsigned char dummy;
    gc(GC_BIT_MASK, mask);
    dummy = vram[off];
    vram[off] = 0xFF;
    (void)dummy;
}

void wait_vsync(void)
{
    while (inp(CRT_STATUS) & 0x08) { }
    while (!(inp(CRT_STATUS) & 0x08)) { }
}

void gfx_init(void)
{
    union REGS r;

    r.x.ax = 0x0010;                 /* 640x350, 16 colours */
    int86(0x10, &r, &r);

    r.h.ah = 0x01;                   /* hide the cursor */
    r.x.cx = 0x2000;
    int86(0x10, &r, &r);

    /* Interrupt 43h holds a far pointer to the character generator for the
       current mode, which the EGA BIOS sets on the mode change -- an 8x14
       font here. Reading the vector avoids INT 10h AH=11h AL=30h, which
       returns its answer in ES:BP and so cannot be called through
       int86x() at all: union REGS has no BP. */
    font = *(const unsigned char far * far *)MK_FP(0x0000, 0x43 * 4);

    /* One palette entry is worth reprogramming. EGA picks its 16 displayed
       colours from 64, encoded six bits per register as rgbRGB: the upper
       three bits are the secondary (one-third) intensities and the lower
       three the primary (two-thirds) ones, so each gun gets four levels.
       The default green at index 2 is primary-only, 0x02 -> (0,170,0),
       which is a loud shade to cover four-fifths of the screen with. Green
       at *secondary* intensity, 0x10, gives (0,85,0) -- a dark felt, and a
       colour plain CGA cannot make at all.

       The suits are left alone: the default bright red, yellow, green and
       blue are already the UNO colours. */
    r.x.ax = 0x1000;
    r.h.bl = GC_FELT;
    r.h.bh = 0x10;
    int86(0x10, &r, &r);

    seq(SEQ_MAP_MASK, 0x0F);         /* writes may reach all four planes */
    gc(GC_MODE, 0x00);               /* write mode 0, read mode 0 */
    gc(GC_DATA_ROTATE, 0x00);        /* replace, no rotate, no logic op */

    gfx_clear(GC_FELT);
}

void gfx_shutdown(void)
{
    union REGS r;
    /* Leave the card in a state DOS can use: default bit mask, Set/Reset
       disabled, and back to 80x25 text. */
    gc(GC_BIT_MASK, 0xFF);
    gc(GC_ENABLE_SET_RESET, 0x00);
    r.x.ax = 0x0003;
    int86(0x10, &r, &r);
}

void gfx_fill_rect(int x, int y, int w, int h, unsigned char color)
{
    int row;
    unsigned int first, last;
    unsigned char lmask, rmask;

    if (w <= 0 || h <= 0) return;
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x >= GFX_W || y >= GFX_H || w <= 0 || h <= 0) return;
    if (x + w > GFX_W) w = GFX_W - x;
    if (y + h > GFX_H) h = GFX_H - y;

    first = (unsigned int)(x >> 3);
    last = (unsigned int)((x + w - 1) >> 3);
    /* Leading and trailing partial bytes; pixel 0 of a byte is bit 7. */
    lmask = (unsigned char)(0xFF >> (x & 7));
    rmask = (unsigned char)(0xFF << (7 - ((x + w - 1) & 7)));

    set_color(color);

    for (row = 0; row < h; row++) {
        unsigned int base = (unsigned int)(y + row) * ROW_BYTES;
        if (first == last) {
            poke_masked(base + first, (unsigned char)(lmask & rmask));
        } else {
            unsigned int i;
            poke_masked(base + first, lmask);
            /* Whole bytes in the middle: every bit comes from Set/Reset, so
               the latches are irrelevant and the read can be skipped. */
            gc(GC_BIT_MASK, 0xFF);
            for (i = first + 1; i < last; i++)
                vram[base + i] = 0xFF;
            poke_masked(base + last, rmask);
        }
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

/* Glyphs land on byte boundaries: x is rounded down to a multiple of 8.
   At 640 pixels that is still 80 text columns, which is more than the
   layout needs, and it keeps a character to one byte-column per row
   instead of straddling two. */
static void glyph(int x, int y, char ch, unsigned char color, int opaque,
                  unsigned char bg)
{
    const unsigned char far *gl = font + (unsigned int)(unsigned char)ch * GFX_FONT_H;
    unsigned int col = (unsigned int)(x >> 3);
    int row;

    if (opaque) gfx_fill_rect((int)(col << 3), y, GFX_FONT_W, GFX_FONT_H, bg);

    set_color(color);
    for (row = 0; row < GFX_FONT_H; row++) {
        unsigned char bits = gl[row];
        int yy = y + row;
        if (bits == 0 || yy < 0 || yy >= GFX_H) continue;
        poke_masked((unsigned int)yy * ROW_BYTES + col, bits);
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

/* ---- verification -------------------------------------------------- */

int gfx_dump(const char *path)
{
    FILE *f = fopen(path, "wb");
    unsigned int plane, row;
    unsigned char buf[ROW_BYTES];

    if (f == NULL) return 1;

    /* A tiny header so the host script doesn't have to assume anything. */
    fputc('E', f); fputc('G', f);
    fputc(GFX_W & 0xFF, f); fputc((GFX_W >> 8) & 0xFF, f);
    fputc(GFX_H & 0xFF, f); fputc((GFX_H >> 8) & 0xFF, f);

    /* Read the planes back one at a time. Reads do not go through the
       latches the way writes go through the ALU: Read Map Select picks
       exactly one plane to appear at A000:0000. */
    gc(GC_ENABLE_SET_RESET, 0x00);
    for (plane = 0; plane < 4; plane++) {
        gc(GC_READ_MAP, (unsigned char)plane);
        for (row = 0; row < GFX_H; row++) {
            unsigned int i, base = row * ROW_BYTES;
            for (i = 0; i < ROW_BYTES; i++) buf[i] = vram[base + i];
            fwrite(buf, 1, ROW_BYTES, f);
        }
    }
    fclose(f);
    return 0;
}
