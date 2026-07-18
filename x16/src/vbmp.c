#include <cx16.h>
#include "vbmp.h"

/* Card action-value codes (from cards.h). Not including cards.h here
   because its COLOR_* suit constants collide with cx16.h's COLOR_* palette
   constants, and this file only needs the value codes. */
#define VAL_SKIP 10
#define VAL_REVERSE 11
#define VAL_DRAW2 12
#define VAL_WILD 13
#define VAL_WILD4 14

/* KERNAL 16-bit-ABI virtual registers (zero page $02-$0A); cx16 cc65 ZP
   starts at $22 so these are ours to set. */
#define R0 (*(volatile unsigned int *)0x02)
#define R1 (*(volatile unsigned int *)0x04)
#define R2 (*(volatile unsigned int *)0x06)
#define R3 (*(volatile unsigned int *)0x08)
#define R4 (*(volatile unsigned int *)0x0A)

/* asm thunks in graph.s */
extern void __fastcall__ kscreen_mode(unsigned char mode);
extern void kgraph_init(void);
extern void kgraph_clear(void);
extern void kgraph_setcolors(void);
extern void __fastcall__ kgraph_rect(unsigned char fill);
extern void __fastcall__ kgraph_oval(unsigned char fill);
extern void __fastcall__ kgraph_putchar(unsigned char ch);

/* read by kgraph_setcolors (must keep these exact names for graph.s) */
unsigned char g_stroke, g_fill, g_bg;

void gfx_palette(unsigned char idx, unsigned char r, unsigned char g, unsigned char b) {
    unsigned long a = 0x1FA00UL + (unsigned long)idx * 2;
    VERA.control = 0;
    VERA.address = (unsigned int)(a & 0xFFFF);
    VERA.address_hi = (unsigned char)(((a >> 16) & 1) | (1 << 4));
    VERA.data0 = (unsigned char)((g << 4) | b);
    VERA.data0 = r;
}

void gfx_init(void) {
    kscreen_mode(0x80);   /* 320x240 @ 256 colours */
    kgraph_init();
    /* custom shades for the table felt and card drop-shadow */
    gfx_palette(GC_FELT, 1, 5, 2);    /* dark green */
    gfx_palette(GC_SHADOW, 2, 2, 2);  /* near-black gray */
}

void gfx_set_colors(unsigned char stroke, unsigned char fill, unsigned char bg) {
    g_stroke = stroke;
    g_fill = fill;
    g_bg = bg;
    kgraph_setcolors();
}

void gfx_clear(void) {
    kgraph_clear();
}

void gfx_rect(unsigned int x, unsigned int y, unsigned int w, unsigned int h, unsigned char fill) {
    R0 = x; R1 = y; R2 = w; R3 = h; R4 = 0;
    kgraph_rect(fill);
}

void gfx_oval(unsigned int x, unsigned int y, unsigned int w, unsigned int h, unsigned char fill) {
    R0 = x; R1 = y; R2 = w; R3 = h;
    kgraph_oval(fill);
}

void gfx_char(unsigned int x, unsigned int y, char c) {
    R0 = x; R1 = y;
    kgraph_putchar((unsigned char)c);
}

void gfx_text(unsigned int x, unsigned int y, const char *s) {
    R0 = x; R1 = y;                 /* put_char advances r0 as it prints */
    while (*s) kgraph_putchar((unsigned char)*s++);
}

/* ---- card art ---- */

#define CARD_W 46
#define CARD_H 64

static unsigned char suit_gc(unsigned char suit) {
    static const unsigned char m[5] = {GC_RED, GC_YELLOW, GC_GREEN, GC_BLUE, GC_LTGRAY};
    return m[suit <= 4 ? suit : 4];
}

static char value_char(unsigned char v) {
    if (v <= 9) return (char)('0' + v);
    switch (v) {
        case VAL_SKIP: return 'S';
        case VAL_REVERSE: return 'R';
        case VAL_DRAW2: return 'D';
        case VAL_WILD: return 'W';
        default: return 'F';
    }
}

void vb_card(unsigned int x, unsigned int y, unsigned char suit, unsigned char value) {
    unsigned char sc = suit_gc(suit);
    char vc = value_char(value);

    /* drop shadow */
    gfx_set_colors(GC_SHADOW, GC_SHADOW, GC_BLACK);
    gfx_rect(x + 3, y + 3, CARD_W, CARD_H, 1);

    /* white card body */
    gfx_set_colors(GC_WHITE, GC_WHITE, GC_BLACK);
    gfx_rect(x, y, CARD_W, CARD_H, 1);

    /* suit-coloured double border */
    gfx_set_colors(sc, sc, GC_WHITE);
    gfx_rect(x + 2, y + 2, CARD_W - 4, CARD_H - 4, 0);
    gfx_rect(x + 3, y + 3, CARD_W - 6, CARD_H - 6, 0);

    /* corner values (top-left, bottom-right). GRAPH_put_char positions a
       character by its BASELINE, so y must clear the glyph height below the
       top edge -- the old y+5 drew the glyph up above the card's top. */
    gfx_char(x + 5, y + 13, vc);
    gfx_char(x + CARD_W - 11, y + CARD_H - 6, vc);

    /* centre pip: suit-coloured oval (visible on the white card body) with
       the value in white */
    gfx_set_colors(sc, sc, GC_WHITE);
    gfx_oval(x + CARD_W / 2 - 12, y + CARD_H / 2 - 14, 24, 28, 1);
    gfx_set_colors(GC_WHITE, GC_WHITE, sc);
    gfx_char(x + CARD_W / 2 - 3, y + CARD_H / 2 + 4, vc);
}

#define HAND_LIFT 12

/* Overlap step: cards fan left-to-right showing each one's left corner-value
   strip. Spread to fit the width, wider for small hands, tighter for big
   ones (so even a 20-card hand stays on screen). */
static int hand_step(unsigned char count) {
    int s;
    if (count <= 1) return 0;
    s = (304 - CARD_W) / (count - 1);
    if (s > 18) s = 18;   /* don't spread cards apart more than this */
    if (s < 11) s = 11;   /* keep at least the corner value visible */
    return s;
}

/* Fan `count` cards starting at (hx,hy); the `selected` card is lifted and
   drawn on top with a highlight frame. Non-selected cards are drawn
   left-to-right so each overlaps the previous, leaving its left strip
   showing. */
void vb_hand(unsigned int hx, unsigned int hy, const unsigned char *suits,
               const unsigned char *values, unsigned char count, unsigned char selected) {
    unsigned char i;
    int step = hand_step(count);
    unsigned int sx;

    for (i = 0; i < count; i++) {
        if (i == selected) continue;
        vb_card(hx + (unsigned int)(i * step), hy, suits[i], values[i]);
    }

    sx = hx + (unsigned int)(selected * step);
    vb_card(sx, hy - HAND_LIFT, suits[selected], values[selected]);
    gfx_set_colors(GC_YELLOW, GC_YELLOW, GC_FELT);
    gfx_rect(sx - 2, hy - HAND_LIFT - 2, CARD_W + 4, CARD_H + 4, 0);
    gfx_rect(sx - 1, hy - HAND_LIFT - 1, CARD_W + 2, CARD_H + 2, 0);
}

void vb_card_back(unsigned int x, unsigned int y) {
    gfx_set_colors(GC_SHADOW, GC_SHADOW, GC_BLACK);
    gfx_rect(x + 3, y + 3, CARD_W, CARD_H, 1);

    gfx_set_colors(GC_WHITE, GC_WHITE, GC_BLACK);
    gfx_rect(x, y, CARD_W, CARD_H, 1);

    gfx_set_colors(GC_RED, GC_RED, GC_WHITE);
    gfx_rect(x + 4, y + 4, CARD_W - 8, CARD_H - 8, 1);

    /* white oval logo with a red "U" (baseline-positioned, see vb_card) */
    gfx_set_colors(GC_WHITE, GC_WHITE, GC_RED);
    gfx_oval(x + CARD_W / 2 - 13, y + CARD_H / 2 - 12, 26, 24, 1);
    gfx_set_colors(GC_RED, GC_RED, GC_WHITE);
    gfx_char(x + CARD_W / 2 - 3, y + CARD_H / 2 + 4, 'U');
}
