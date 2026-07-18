#include <string.h>
#include "mega65spr.h"

#define POKE(a, v) (*(volatile unsigned char *)(a) = (unsigned char)(v))

#define VIC_SPR0_X   0xD000
#define VIC_SPR0_Y   0xD001
#define VIC_SPR_MSB  0xD010   /* X bit 8 per sprite */
#define VIC_SPR_ENABLE 0xD015
#define VIC_SPR0_COLOR 0xD027
#define VIC_SPR_XMSBS 0xD05F  /* MEGA65 X bit 9 per sprite (kept clear here) */
#define VIC_SPR_YMSB 0xD077   /* MEGA65 Y bit 8 per sprite (kept clear here) */

#define SPR_PTR      0x07F8   /* sprite 0 pointer (screen $0400 + $3F8) */
#define SPR_DATA     0x0340   /* tape buffer, safe in VIC bank 0 */

/* 24x21 filled card silhouette with trimmed corners (same shape as the C64
   port's toss sprite). Colour is set per toss. */
static const unsigned char spr_shape[63] = {
    0x00, 0x00, 0x00,
    0x0F, 0xFF, 0xF0,
    0x3F, 0xFF, 0xFC, 0x3F, 0xFF, 0xFC, 0x3F, 0xFF, 0xFC, 0x3F, 0xFF, 0xFC,
    0x3F, 0xFF, 0xFC, 0x3F, 0xFF, 0xFC, 0x3F, 0xFF, 0xFC, 0x3F, 0xFF, 0xFC,
    0x3F, 0xFF, 0xFC, 0x3F, 0xFF, 0xFC, 0x3F, 0xFF, 0xFC, 0x3F, 0xFF, 0xFC,
    0x3F, 0xFF, 0xFC, 0x3F, 0xFF, 0xFC, 0x3F, 0xFF, 0xFC,
    0x0F, 0xFF, 0xF0,
    0x00, 0x00, 0x00
};

void spr_init(void) {
    memcpy((void *)SPR_DATA, spr_shape, 63);
    POKE(SPR_PTR, SPR_DATA / 64);       /* 0x0340/64 = 0x0D */
    POKE(VIC_SPR_ENABLE, 0x00);
}

/* Character cell -> VIC-II sprite pixel coordinates (standard C64 origin:
   first text column at x=24, first row at y=50). */
static void set_pos(unsigned char col, unsigned char row) {
    unsigned int px = 24 + (unsigned int)col * 8;
    unsigned char py = (unsigned char)(50 + (unsigned int)row * 8);
    POKE(VIC_SPR0_X, px & 0xFF);
    if (px > 255) POKE(VIC_SPR_MSB, 0x01);
    else POKE(VIC_SPR_MSB, 0x00);
    POKE(VIC_SPR_XMSBS, 0x00);
    POKE(VIC_SPR0_Y, py);
    POKE(VIC_SPR_YMSB, 0x00);
}

void spr_show(unsigned char color, unsigned char col, unsigned char row) {
    POKE(VIC_SPR0_COLOR, color);
    set_pos(col, row);
    POKE(VIC_SPR_ENABLE, 0x01);
}

void spr_move(unsigned char col, unsigned char row) {
    set_pos(col, row);
}

void spr_hide(void) {
    POKE(VIC_SPR_ENABLE, 0x00);
}
