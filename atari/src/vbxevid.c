#include <string.h>
#include "vbxevid.h"

/* Register base $D6xx: the more common of the two supported addresses
   ($D6xx or $D7xx) per VBXE's own docs -- most VBXE software assumes
   this one and won't adapt to $D7xx, so this doesn't bother making it
   configurable. */
#define VBXE_BASE 0xD640
#define VIDEO_CONTROL (*(unsigned char *)(VBXE_BASE + 0x00))
#define XDL_ADR0 (*(unsigned char *)(VBXE_BASE + 0x01))
#define XDL_ADR1 (*(unsigned char *)(VBXE_BASE + 0x02))
#define XDL_ADR2 (*(unsigned char *)(VBXE_BASE + 0x03))
/* Palette-programming registers as used by the FX core (and by every
   working reference: Cactus, ansivbxe, Piotr Fusik's st2vbxe). NOTE: the
   older VBXE manual documents these same addresses as an MSEL/MB0-3
   "commit" protocol, but the FX core actually exposes them as direct
   CSEL/PSEL/CR/CG/CB registers -- writing them the manual's way scrambles
   the palette (index/RGB land in the wrong slots), which renders text in
   a colour indistinguishable from its background (looks like blank
   glyphs) and gives wrong backgrounds. */
#define CSEL (*(unsigned char *)(VBXE_BASE + 0x04)) /* colour index 0-255 */
#define PSEL (*(unsigned char *)(VBXE_BASE + 0x05)) /* palette 0 or 1 */
#define CR (*(unsigned char *)(VBXE_BASE + 0x06))   /* red   */
#define CG (*(unsigned char *)(VBXE_BASE + 0x07))   /* green */
#define CB (*(unsigned char *)(VBXE_BASE + 0x08))   /* blue  */

/* MEMAC window A, FX-core protocol ($D65E/$D65F), confirmed against
   Altirra's own vbxe.cpp. The old v1.0-beta manual's MA_CPU register at
   $D64C DOES NOT EXIST on the FX core -- Altirra's register-write switch
   has no case for $4C at all, so writes there are silently dropped and
   the window never opens (every "VRAM write" went to plain Atari RAM,
   which also made same-window read-backs look correct).
     MEMAC_CONTROL : high nibble = CPU window base page ($X000),
                     bit3 = CPU access enable, bit2 = ANTIC access enable,
                     bits0-1 = window size (4K << n)
     MEMAC_BANK_SEL: bit7 = enable, bits0-6 = VRAM bank in 4KB units
                     (masked to the window size, e.g. & $7E for 8K) */
#define MEMAC_CONTROL (*(unsigned char *)(VBXE_BASE + 0x1E))
#define MEMAC_BANK_SEL (*(unsigned char *)(VBXE_BASE + 0x1F))

#define VIC_RASTER (*(unsigned char *)0xD40B)  /* ANTIC VCOUNT, still ticking under VBXE */

/* MEMAC window A, configured by select_bank() as an 8KB window at
   $2000-$3FFF reaching VBXE's 512KB VRAM from the CPU. Enabling it banks
   VBXE VRAM in over whatever normally lives at $2000-$3FFF -- including
   this program's own code, if cc65 had put it there ($2400 is the atarixl
   default). atari/Makefile links this target with --start-addr 0x4000 so
   the running program (and its stack) sits entirely above the window and
   never gets banked out from under itself mid-instruction. (cc65's linker
   emits a harmless warning that its startup code reaches into $4000-$7FFF,
   which it treats as a RAMBO-style bank window -- this driver never
   enables MEMAC B there, so that range is plain RAM here.) */
#define MEMAC_A ((unsigned char *)0x2000)
#define MEMAC_A_SIZE 0x2000UL

/* The whole screen lives in MEMAC bank 0 (VRAM $0000-$1FFF), so once bank 0
   is selected it is directly addressable in the CPU window at
   $2000 + (vram & $1FFF). SCREEN_WIN is that CPU pointer for VRAM_SCREEN.
   Using it (with plain unsigned int offsets) instead of vram_write_byte's
   per-byte unsigned-long divide/modulo is the difference between a screen
   clear taking ~15 frames and ~600 -- the long math per byte made
   vbxe_init appear to hang. */
#define SCREEN_WIN ((unsigned char *)(0x2000U + (VRAM_SCREEN & 0x1FFFU)))

/* VRAM layout, chosen by this driver (VBXE imposes no fixed map). Kept
   ENTIRELY inside MEMAC bank 0 (VRAM $0000-$1FFF):
     $00000  XDL
     $00800  font, 2KB (256 chars x 8 bytes; VBXE requires a 2KB-aligned
             base; CHBASE = $0800>>11 = 1)
     $01000  screen (char+attribute pairs, COLS*ROWS*2 = 3840 bytes,
             ends $1EFF)

   Everything is in bank 0 deliberately: MEMAC-A writes to bank 0 are
   verified working, but writes to bank 1 ($2000+) did NOT land in VBXE
   VRAM here -- with the font at $2000 (bank 1) the OVADR-at-font test
   showed the font region reading as all zeros, so glyphs came out blank.
   Moving the font into bank 0 sidesteps the bank-1 write problem. (CHBASE
   is 1, not 0: a zero font base is separately suspect -- the manual's
   text-mode example never uses one.) */
#define VRAM_XDL 0x00000UL
#define VRAM_FONT 0x00800UL
#define VRAM_SCREEN 0x01000UL

static unsigned char current_bank = 0xFF; /* force the first select */

static void select_bank(unsigned long vram_addr) {
    /* Bank field is in 4KB units; for the 8K window the low bit is
       ignored (mask $7E), so step in 2s per 8K region. */
    unsigned char bank = (unsigned char)((vram_addr >> 12) & 0x7E);
    if (bank != current_bank) {
        MEMAC_CONTROL = 0x29; /* window at $2000, CPU enable, 8K */
        MEMAC_BANK_SEL = (unsigned char)(0x80 | bank);
        current_bank = bank;
    }
}

/* Writes len bytes to VBXE VRAM starting at vram_addr, through the
   MEMAC A window, crossing bank boundaries as needed. */
static void vram_write(unsigned long vram_addr, const unsigned char *data, unsigned int len) {
    unsigned int off;
    for (off = 0; off < len; off++) {
        unsigned long addr = vram_addr + off;
        select_bank(addr);
        MEMAC_A[addr % MEMAC_A_SIZE] = data[off];
    }
}

static void vram_write_byte(unsigned long vram_addr, unsigned char value) {
    select_bank(vram_addr);
    MEMAC_A[vram_addr % MEMAC_A_SIZE] = value;
}

unsigned char vram_read_byte(unsigned long vram_addr) {
    select_bank(vram_addr);
    return MEMAC_A[vram_addr % MEMAC_A_SIZE];
}

/* Streams `count` consecutive RGB triples into Overlay palette 1 starting
   at colour index `start`. Matches Piotr Fusik's st2vbxe: set PSEL(=1) and
   CSEL(=start) ONCE, then write CR/CG/CB per colour -- the FX core auto-
   advances the colour index after each triple, so CSEL is not re-set per
   entry (doing so per-entry is what appears to scramble the palette). */
static void stream_palette(unsigned char start, const unsigned char *rgb, unsigned char count) {
    unsigned char i;
    PSEL = 1;
    CSEL = start;
    for (i = 0; i < count; i++) {
        CR = *rgb++;
        CG = *rgb++;
        CB = *rgb++;
    }
}

void wait_vsync(void) {
    /* ANTIC's VCOUNT (half-scanline count, wraps every field) keeps
       running under VBXE since the Overlay rides on top of the normal
       ANTIC/GTIA timing rather than replacing it -- same idea as the
       C128 VDC port's wait_vsync() still reading the VIC-II raster. */
    unsigned char start = VIC_RASTER;
    while (VIC_RASTER == start) {
    }
}

void vbxe_init(void) {
    /* The real Atari ROM character set: CHBAS ($02F4) holds its base
       page, page*256 = the actual address (normally $E0 -> $E000 at
       boot). Reading this instead of hardcoding $E000 costs nothing
       and is the documented, correct way to find it. */
    unsigned char chbas = *(unsigned char *)0x02F4;
    unsigned char *rom_font = (unsigned char *)((unsigned int)chbas << 8);

    vram_write(VRAM_FONT, rom_font, 1024);
    vram_write(VRAM_FONT + 1024, rom_font, 1024); /* codes 128-255: unused, just a spare copy */

    /* Palette 0 (ANTIC/GTIA/P-M's default) is left at VBXE's factory
       colors; only palette 1 (the Overlay's default) is programmed,
       since nothing here draws through ANTIC/GTIA directly.

       In the b7=1 attribute encoding this driver uses (make_attr()),
       background colour is NOT automatically black -- it is its own
       independent palette index, (foreground index) + 128. Every
       background index this driver can produce (128 + each COL_*
       constant, since every character is written with its own color
       as both an implicit foreground and, via +128, background) has
       to be explicitly programmed to black, or it shows whatever
       garbage happens to be in that unprogrammed palette slot (which
       is exactly the all-white screen this was empirically observed
       to produce). */
    {
        /* Foreground colours: indices 0..COL_ORANGE, streamed in one run.
           Index 0 (COL_BLACK) explicitly black; 1..8 the suit/UI colours,
           in COL_* order. */
        static const unsigned char fg_rgb[] = {
            0, 0, 0,        /* 0 COL_BLACK  */
            255, 255, 255,  /* 1 COL_WHITE  */
            255, 40, 40,    /* 2 COL_RED    */
            255, 230, 40,   /* 3 COL_YELLOW */
            60, 220, 60,    /* 4 COL_GREEN  */
            80, 120, 255,   /* 5 COL_BLUE   */
            60, 220, 220,   /* 6 COL_CYAN   */
            230, 60, 220,   /* 7 COL_MAGENTA*/
            255, 150, 40    /* 8 COL_ORANGE */
        };
        /* Backgrounds: indices 128..128+COL_ORANGE, all black (the b7=1
           attribute encoding puts a cell's background at fg+128). */
        static const unsigned char bg_rgb[(COL_ORANGE + 1) * 3] = {0};
        stream_palette(0, fg_rgb, COL_ORANGE + 1);
        stream_palette(128, bg_rgb, COL_ORANGE + 1);
    }

    scr_clear();

    /* XDL: two entries, mirroring the structure of a known-working VBXE
       text-mode implementation (the Cactus browser's setup_xdl).

       The critical detail this encodes: CHBASE (the font pointer) and
       OVATT are set in a FIRST entry, with the overlay OFF (XDLC_OVOFF),
       covering an 8-scanline top border -- and the SECOND entry (the one
       that actually turns text mode on, XDLC_TMON) does NOT touch CHBASE.
       Setting CHBASE in the same entry as TMON produced entirely blank
       glyphs under Altirra (correct backgrounds, zero foreground pixels)
       even with byte-perfect font/screen/XDL data: the overlay fetches
       glyphs using the CHBASE established by a PRIOR entry, so CHBASE has
       to be programmed before the text-mode entry runs. Backgrounds
       don't depend on CHBASE, which is why they rendered fine while the
       glyphs stayed blank -- exactly the symptom this fixes.

       During the OVOFF border, OVADR still auto-advances, so entry 2
       re-sets OVADR back to the top of the screen. A single XDLC_RPTL of
       ROWS*8-1 on entry 2 then covers all 24 text rows (OVADR auto-steps
       by OVSTEP every 8 scanlines in text mode), which does work here --
       the earlier "single entry only covered ~10 rows" symptom was
       really this same CHBASE-timing bug, not an RPTL-coverage limit. */
    {
        unsigned char entry[11];
        unsigned char n;
        unsigned int screen_step = (unsigned int)COLS * 2; /* bytes per text row */
        unsigned long xdl_off = VRAM_XDL;
        unsigned long ovadr = VRAM_SCREEN;

        /* Entry 1: 8-scanline border, overlay OFF, but programs OVADR +
           CHBASE + OVATT so they're in effect before the text entry.
           Low ctrl byte: OVOFF=0x04 | MAPOFF=0x10 | RPTL=0x20 | OVADR=0x40.
           High ctrl byte: CHBASE=0x01 | OVATT=0x08. */
        n = 0;
        entry[n++] = 0x04 | 0x10 | 0x20 | 0x40;
        entry[n++] = 0x01 | 0x08;
        entry[n++] = 7; /* RPTL: hold 7 more scanlines (8 total) */
        entry[n++] = (unsigned char)(ovadr & 0xFF);              /* OVADR addr */
        entry[n++] = (unsigned char)((ovadr >> 8) & 0xFF);
        entry[n++] = (unsigned char)((ovadr >> 16) & 0x07);
        entry[n++] = (unsigned char)(screen_step & 0xFF);        /* OVADR step */
        entry[n++] = (unsigned char)((screen_step >> 8) & 0x0F);
        entry[n++] = (unsigned char)(VRAM_FONT >> 11);           /* CHBASE (2KB pages) */
        /* OVATT byte 1 (per Mad Pascal / Cactus): bits 0-1 = width
           (01 = NORMAL, 320px / 80 cols), bits 4-5 = Overlay palette
           select. This driver programs palette 1 (PSEL=1 in set_palette),
           so bit 4 MUST be set (%00010001 = 0x11) to make the overlay
           read palette 1. With 0x01 (palette 0) the overlay read the
           unprogrammed default palette -> wrong colours, foreground
           indistinguishable from background (text looked blank). */
        entry[n++] = 0x11;
        entry[n++] = 255; /* OVATT byte 2: priority, overlay over everything */
        vram_write(xdl_off, entry, n);
        xdl_off += n;

        /* Entry 2: text mode for all ROWS*8 scanlines, re-set OVADR to the
           top of the screen, then END. Byte-for-byte the structure of
           Cactus's working 80-column text XDL. Low ctrl: TMON=0x01 |
           MAPOFF=0x10 | RPTL=0x20 | OVADR=0x40; high ctrl: END=0x80. */
        n = 0;
        entry[n++] = 0x01 | 0x10 | 0x20 | 0x40;
        entry[n++] = 0x80;
        entry[n++] = (unsigned char)(ROWS * 8 - 1);
        entry[n++] = (unsigned char)(ovadr & 0xFF);
        entry[n++] = (unsigned char)((ovadr >> 8) & 0xFF);
        entry[n++] = (unsigned char)((ovadr >> 16) & 0x07);
        entry[n++] = (unsigned char)(screen_step & 0xFF);
        entry[n++] = (unsigned char)((screen_step >> 8) & 0x0F);
        vram_write(xdl_off, entry, n);
    }

    XDL_ADR0 = (unsigned char)(VRAM_XDL & 0xFF);
    XDL_ADR1 = (unsigned char)((VRAM_XDL >> 8) & 0xFF);
    XDL_ADR2 = (unsigned char)((VRAM_XDL >> 16) & 0x07);

    /* VIDEO_CONTROL: xdl_enabled=1 (bit0), no_trans=1 (bit2, so every
       one of the 128 foreground indices is usable without index 0
       meaning "transparent" -- simpler than tracking which colors are
       safe to use for a background fill). */
    VIDEO_CONTROL = 0x01 | 0x04;

    /* Disable ANTIC playfield DMA (SDMCTL, the OS shadow of DMACTL that
       the VBI copies to $D400): the VBXE Overlay covers the whole screen
       here, so the stock ANTIC display underneath is just noise bleeding
       through the OVOFF top border. Matches Cactus's setup. */
    *(unsigned char *)0x022F = 0;
}

/* Packs a character cell: attribute byte b7=1 (opaque) format, fg in
   bits 0-6, background is its own independent palette index (fg+128),
   separately zeroed to black for every fg index this driver uses --
   see the palette-programming loop in vbxe_init(). */
static unsigned char make_attr(unsigned char color) {
    return (unsigned char)(0x80 | (color & 0x7F));
}

/* The font copied into VBXE VRAM is a straight dump of the OS ROM
   character generator (CHBAS), which is indexed by Atari's internal
   "screen code" order, not ATASCII -- the same reason plain screen-
   RAM pokes on a stock Atari need this translation. cc65's conio.h
   (used by atarivid.c's scr_put/scr_puts) does this for you under the
   hood via the OS screen editor; writing straight into VBXE VRAM
   bypasses the OS entirely, so this driver has to do it itself.
   Standard mapping for the printable ASCII/ATASCII range this game
   actually uses (space, digits, uppercase, punctuation): codes 0x20-
   0x5F map to screen codes 0x00-0x3F; codes below 0x20 (unused here)
   map to 0x40-0x5F. */
static unsigned char to_screen_code(unsigned char c) {
    if (c < 0x20) return (unsigned char)(c + 0x40);
    if (c < 0x60) return (unsigned char)(c - 0x20);
    return c;
}

void scr_clear(void) {
    unsigned int i;
    unsigned char *p;
    unsigned char sc = to_screen_code(' ');
    unsigned char at = make_attr(COL_BLACK);
    select_bank(VRAM_SCREEN);           /* bank 0, once */
    p = SCREEN_WIN;
    for (i = 0; i < (unsigned int)COLS * ROWS; i++) {
        *p++ = sc;
        *p++ = at;
    }
}

void scr_put(unsigned char x, unsigned char y, unsigned char ch, unsigned char color) {
    unsigned char *p = SCREEN_WIN + ((unsigned int)y * COLS + x) * 2;
    select_bank(VRAM_SCREEN);
    p[0] = to_screen_code(ch);
    p[1] = make_attr(color);
}

void scr_puts(unsigned char x, unsigned char y, const char *s, unsigned char color) {
    unsigned char *p = SCREEN_WIN + ((unsigned int)y * COLS + x) * 2;
    unsigned char attr = make_attr(color);
    select_bank(VRAM_SCREEN);
    while (*s) {
        *p++ = to_screen_code((unsigned char)*s++);
        *p++ = attr;
    }
}

void scr_put_num(unsigned char x, unsigned char y, unsigned int n, unsigned char color) {
    char buf[6];
    unsigned char i = 0;
    if (n == 0) {
        scr_put(x, y, '0', color);
        return;
    }
    while (n > 0 && i < 5) {
        buf[i++] = (char)('0' + (n % 10));
        n /= 10;
    }
    while (i > 0) {
        i--;
        scr_put(x, y, (unsigned char)buf[i], color);
        x++;
    }
}

void scr_fill_rect(unsigned char x, unsigned char y, unsigned char w, unsigned char h,
                    unsigned char ch, unsigned char color) {
    unsigned char row, col;
    unsigned char attr = make_attr(color);
    unsigned char screen_ch = to_screen_code(ch);
    select_bank(VRAM_SCREEN);
    for (row = 0; row < h; row++) {
        unsigned char *p = SCREEN_WIN + ((unsigned int)(y + row) * COLS + x) * 2;
        for (col = 0; col < w; col++) {
            *p++ = screen_ch;
            *p++ = attr;
        }
    }
}
