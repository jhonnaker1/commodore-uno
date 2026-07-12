#ifndef COCOVID_H
#define COCOVID_H

/* CoCo 3, 40-column Super Extended Color BASIC text mode. Per the GIME's
   own hardware docs, the text attribute byte's foreground and background
   fields are NOT the same 8-colour palette read twice -- foreground
   indices (0-7) map to palette registers 8-15, background indices (0-7)
   map to registers 0-7, entirely independent banks. This wasn't obvious
   from CMOC's coco.h alone (confirmed by testing attr() combinations in
   XRoar, then cross-checked against Lomont's CoCo hardware reference),
   and it means every foreground colour below is explicitly programmed
   via paletteRGB() in coco_init() rather than left at whatever the boot
   default happened to be -- true per-suit colour (unlike the PET/VIC-20/
   Atari/ZX Spectrum ports, which fake colour with a bracketed
   [label:COLORLETTER+VALUE] scheme since their hardware can't cheaply do
   per-character colour at all). CoCo3's text mode has no redefinable
   character generator though (confirmed against the same hardware
   reference), so unlike the C64 port, cards are still bracket/letter
   text, just genuinely coloured by suit now instead of a single ink. */
#define COLS 40
#define ROWS 24

#define COL_RED 0
#define COL_YELLOW 1
#define COL_GREEN 2
#define COL_BLUE 3
#define COL_NORMAL 4
#define COL_WILD 5
#define COL_SELECTED 6
#define COL_ALERT 7

void coco_init(void);
void wait_vsync(void);
void scr_clear(void);
void scr_put(unsigned char x, unsigned char y, char ch, unsigned char color);
void scr_puts(unsigned char x, unsigned char y, const char *s, unsigned char color);
void scr_put_num(unsigned char x, unsigned char y, unsigned int n);
void scr_fill_rect(unsigned char x, unsigned char y, unsigned char w, unsigned char h);

#endif
