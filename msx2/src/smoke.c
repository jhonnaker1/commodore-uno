/* Pipeline smoke test: prove that an SDCC-built cartridge boots on an MSX2,
   that SCREEN 5 comes up, that the palette is really programmable, and that
   the V9938's command engine draws. Not part of the game -- `make smoke`. */

__sfr __at 0x98 VDP_DATA;
__sfr __at 0x99 VDP_ADDR;
__sfr __at 0x9A VDP_PAL;
__sfr __at 0x9B VDP_REGI;

extern void bios_chgmod(unsigned char mode);

/* Plain di/ei, not SDCC's __critical -- see the long note in msxvdp.c:
   __critical's `ld a,i` state save hits a Z80 erratum that can leave
   interrupts off permanently. */
#define IRQ_OFF() __asm di __endasm
#define IRQ_ON()  __asm ei __endasm

/* Both bytes of a register write go to the same port back to back; an
   interrupt landing between them would leave the VDP's latch half-set and
   the next write would be interpreted as the other half. */
static void vdp_reg(unsigned char reg, unsigned char val) {
    IRQ_OFF();
    VDP_ADDR = val;
    VDP_ADDR = 0x80 | reg;
    IRQ_ON();
}

static void vdp_palette(unsigned char idx, unsigned char r, unsigned char g,
                        unsigned char b) {
    vdp_reg(16, idx);
    VDP_PAL = (r << 4) | b;   /* R and B share a byte, 3 bits each */
    VDP_PAL = g;
}

/* S#2 bit 0 (CE) stays set while the command engine is busy.
   R#15 selects which status register port 0x99 reads -- and the BIOS
   interrupt handler reads S#0 every frame to acknowledge the VDP interrupt,
   so leaving R#15 pointing anywhere else would break the interrupt for good.
   Select, read, and put it straight back, with interrupts off throughout. */
static void vdp_wait_cmd(void) {
    unsigned char s;
    for (;;) {
        __critical {
            VDP_ADDR = 2;
            VDP_ADDR = 0x80 | 15;
            s = VDP_ADDR;
            VDP_ADDR = 0;
            VDP_ADDR = 0x80 | 15;
        }
        if (!(s & 0x01)) return;
    }
}

/* HMMV: fill a rectangle with a colour, VDP-side.
   Coordinates are PIXELS, not bytes. The V9938 manual describes the
   high-speed commands as working in byte units, but that is about the
   engine's internal transfer granularity, not the numbers you feed it --
   every command takes pixel coordinates. What byte granularity really
   costs you here is that HMMV cannot start or stop on an odd pixel: the
   low bit of x and w is ignored, since one byte is two pixels in SCREEN 5.
   R#17 is the indirect-register pointer: point it at R#32 and every write
   to port 0x9B lands in the next command register, so the whole 15-byte
   command block goes out as a straight run. Writing R#46 starts the fill. */
static void vdp_fill(unsigned int x, unsigned int y, unsigned int w,
                     unsigned int h, unsigned char color) {
    vdp_wait_cmd();
    vdp_reg(17, 32);
    VDP_REGI = 0; VDP_REGI = 0;                    /* R#32/33 SX (unused) */
    VDP_REGI = 0; VDP_REGI = 0;                    /* R#34/35 SY (unused) */
    VDP_REGI = x & 0xFF; VDP_REGI = x >> 8;        /* R#36/37 DX */
    VDP_REGI = y & 0xFF; VDP_REGI = y >> 8;        /* R#38/39 DY */
    VDP_REGI = w & 0xFF; VDP_REGI = w >> 8;        /* R#40/41 NX */
    VDP_REGI = h & 0xFF; VDP_REGI = h >> 8;        /* R#42/43 NY */
    VDP_REGI = (color << 4) | color;               /* R#44 both pixels */
    VDP_REGI = 0;                                  /* R#45 left/down */
    VDP_REGI = 0xC0;                               /* R#46 HMMV */
}

void main(void) {
    unsigned char i;

    bios_chgmod(5);

    /* Deliberately not the default palette: if these take, the colours on
       screen are ones an MSX1 physically cannot produce. */
    vdp_palette(0, 0, 0, 0);   /* black    */
    vdp_palette(1, 7, 7, 7);   /* white    */
    vdp_palette(2, 7, 0, 0);   /* red      */
    vdp_palette(3, 7, 6, 0);   /* yellow   */
    vdp_palette(4, 0, 6, 1);   /* green    */
    vdp_palette(5, 1, 2, 7);   /* blue     */
    vdp_palette(6, 0, 2, 0);   /* felt     */

    /* R#7's low nibble is the backdrop, which paints the border outside the
       212-line active area. Left alone it stays whatever the boot screen
       used, framing the game in an unrelated colour. */
    vdp_reg(7, 6);

    vdp_fill(0, 0, 256, 212, 6);
    for (i = 0; i < 4; i++)
        vdp_fill(16 + i * 60, 40, 48, 72, (unsigned char)(2 + i));
    vdp_fill(16, 140, 224, 24, 1);

    for (;;) { }
}
