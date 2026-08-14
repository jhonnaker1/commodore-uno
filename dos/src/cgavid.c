#include <dos.h>
#include <conio.h>
#include "cgavid.h"

/* The text framebuffer. Real mode, so this is a plain far pointer at the
   hardware address -- no mapping, no bank switching, no port protocol.
   After the MSX2's VDP and the ST's bitplanes this is a holiday. */
static unsigned char far *vram = (unsigned char far *)MK_FP(0xB800, 0x0000);

/* CGA I/O. 0x3D8 is the mode control register, 0x3DA the status register;
   both are still present on EGA and VGA, which is why wait_vsync() works
   everywhere even though the mode register does not. */
#define CGA_MODE   0x3D8
#define CGA_STATUS 0x3DA

/* 80x25 text, video enabled, blink disabled:
   bit0 = 80-column text, bit3 = video enable, bit5 = blink enable (clear). */
#define CGA_MODE_80X25_NOBLINK 0x09

static int is_ega_or_better = 0;

/* Turning blinking off is what buys the bright backgrounds, and the two
   generations of hardware want it done differently:

     - On EGA/VGA the blink bit lives in the attribute controller, and the
       BIOS exposes it as INT 10h AX=1003h. Poking CGA's mode register on a
       VGA is not reliable -- the register is not part of the VGA spec.
     - On a real CGA there is no such BIOS call, and the bit is bit 5 of the
       mode register at 0x3D8, which has to be written along with the rest
       of the mode.

   So detect which we are on (INT 10h AH=12h/BL=10h leaves BL alone on
   anything that is not EGA or better) and use the right one. */
static void disable_blink(void)
{
    union REGS r;

    r.h.ah = 0x12;
    r.h.bl = 0x10;
    int86(0x10, &r, &r);
    is_ega_or_better = (r.h.bl != 0x10);

    if (is_ega_or_better) {
        r.x.ax = 0x1003;
        r.h.bl = 0x00;          /* 0 = intensity, 1 = blink */
        r.h.bh = 0x00;
        int86(0x10, &r, &r);
    } else {
        outp(CGA_MODE, CGA_MODE_80X25_NOBLINK);
    }
}

void cga_init(void)
{
    union REGS r;

    r.x.ax = 0x0003;            /* 80x25 16-colour text */
    int86(0x10, &r, &r);

    /* Hide the hardware cursor: setting the start scanline above the end
       is the documented way, and unlike moving it off-screen it survives a
       later BIOS call that repositions the cursor. */
    r.h.ah = 0x01;
    r.x.cx = 0x2000;
    int86(0x10, &r, &r);

    disable_blink();
    scr_clear(ATTR(C_LGRAY, C_BLACK));
}

void cga_shutdown(void)
{
    union REGS r;

    /* Put blinking and the cursor back before returning to DOS, or the
       command prompt inherits an invisible cursor and odd attributes. */
    if (is_ega_or_better) {
        r.x.ax = 0x1003;
        r.h.bl = 0x01;
        r.h.bh = 0x00;
        int86(0x10, &r, &r);
    }
    r.x.ax = 0x0003;
    int86(0x10, &r, &r);
}

void wait_vsync(void)
{
    /* Wait out any retrace already in progress, then catch the leading edge
       of the next one -- otherwise a call landing mid-retrace returns
       immediately and the caller gets a short frame. */
    while (inp(CGA_STATUS) & 0x08) { }
    while (!(inp(CGA_STATUS) & 0x08)) { }
}

/* ---- CGA snow ------------------------------------------------------
   The 6845 and the CPU share the regen buffer with no arbitration. A CPU
   access while the card is actively fetching display data steals the cycle
   the CRTC needed, and the card shows garbage for it -- the coloured
   speckling known as snow. It afflicts the 80-column text modes on genuine
   IBM CGA; clones fixed it, and EGA/VGA do not have the problem at all.

   Bit 0 of the status port is 1 exactly when a regen-buffer access can be
   made without disturbing the display. Waiting out any window already in
   progress before catching the start of the next one means the write lands
   with a whole interval ahead of it rather than at the tail of one.

   This costs a poll loop per cell, so it is done only where it is needed:
   is_ega_or_better is already worked out at init for the blink setting, and
   anything EGA or later writes at full speed. */
static void poke_cell(unsigned int off, unsigned char ch, unsigned char attr)
{
    if (is_ega_or_better) {
        vram[off] = ch;
        vram[off + 1] = attr;
        return;
    }
    while (inp(CGA_STATUS) & 0x01) { }
    while (!(inp(CGA_STATUS) & 0x01)) { }
    vram[off] = ch;
    vram[off + 1] = attr;
}

void scr_clear(unsigned char attr)
{
    unsigned int i;
    for (i = 0; i < COLS * ROWS; i++)
        poke_cell(i * 2, ' ', attr);
}

void scr_put(int x, int y, char ch, unsigned char attr)
{
    if (x < 0 || y < 0 || x >= COLS || y >= ROWS) return;
    poke_cell((unsigned int)((y * COLS + x) * 2), (unsigned char)ch, attr);
}

void scr_puts(int x, int y, const char *s, unsigned char attr)
{
    while (*s && x < COLS) {
        scr_put(x++, y, *s++, attr);
    }
}

void scr_fill(int x, int y, int w, int h, char ch, unsigned char attr)
{
    int i, j;
    for (j = 0; j < h; j++)
        for (i = 0; i < w; i++)
            scr_put(x + i, y + j, ch, attr);
}

void scr_put_num(int x, int y, unsigned int n, unsigned char attr)
{
    char buf[6];
    int i = 0, j;
    if (n == 0) { scr_put(x, y, '0', attr); return; }
    while (n && i < 5) { buf[i++] = (char)('0' + (n % 10)); n /= 10; }
    for (j = 0; j < i; j++) scr_put(x + j, y, buf[i - 1 - j], attr);
}
