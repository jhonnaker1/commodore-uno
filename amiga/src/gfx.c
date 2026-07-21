#include <intuition/intuition.h>
#include <graphics/gfx.h>
#include <graphics/rastport.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include "gfx.h"
#include "amigacon.h"   /* shared con_getkey()/wait_vsync() contract with input.c */

/* card action-value codes (from cards.h; kept local so this stays a pure
   video layer) */
#define VAL_SKIP 10
#define VAL_REVERSE 11
#define VAL_DRAW2 12
#define VAL_WILD 13
#define VAL_WILD4 14

/* topaz 8 baseline: Text() positions a glyph by its baseline, 6 pixels down
   from the cell top, so add this to a top-left y. */
#define FONT_BASE 6

static struct NewScreen ns = {
    0, 0, GFX_W, GFX_H, 4,          /* depth 4 -> 16 colours */
    0, 1,
    0,                              /* lores, non-interlaced */
    CUSTOMSCREEN,
    NULL, (UBYTE *)"UNO", NULL, NULL
};

static struct NewWindow nw = {
    0, 0, GFX_W, GFX_H,
    0, 1,
    IDCMP_VANILLAKEY,
    ACTIVATE | BORDERLESS | BACKDROP | RMBTRAP,
    NULL, NULL,
    NULL,
    NULL, NULL,
    0, 0, 0, 0,
    CUSTOMSCREEN
};

static struct Screen *scr;
static struct Window *win;
static struct RastPort *rp;

/* suit (0-4) -> palette index */
static const unsigned char suit_col[5] = {GC_RED, GC_YELLOW, GC_GREEN, GC_BLUE, GC_GRAY};

void gfx_init(void) {
    scr = OpenScreen(&ns);

    SetRGB4(&scr->ViewPort, GC_BLACK, 0, 0, 0);
    SetRGB4(&scr->ViewPort, GC_WHITE, 15, 15, 15);
    SetRGB4(&scr->ViewPort, GC_RED, 13, 2, 2);
    SetRGB4(&scr->ViewPort, GC_GREEN, 2, 11, 4);
    SetRGB4(&scr->ViewPort, GC_BLUE, 4, 6, 14);
    SetRGB4(&scr->ViewPort, GC_YELLOW, 14, 12, 2);
    SetRGB4(&scr->ViewPort, GC_GRAY, 9, 9, 9);
    SetRGB4(&scr->ViewPort, GC_SHADOW, 1, 3, 2);
    SetRGB4(&scr->ViewPort, GC_FELT, 1, 6, 3);

    nw.Screen = scr;
    win = OpenWindow(&nw);
    rp = win->RPort;

    /* Hide the custom screen's title bar so the full 256 rows are ours --
       otherwise it sits over the top of the playfield. */
    ShowTitle(scr, FALSE);

    SetDrMd(rp, JAM1);              /* text: draw foreground pen only */
    gfx_clear(GC_FELT);
}

void gfx_shutdown(void) {
    if (win) CloseWindow(win);
    if (scr) CloseScreen(scr);
}

/* No memory-mapped raster register here; pace with the display's own
   top-of-frame wait (one PAL frame = 1/50s). */
void wait_vsync(void) {
    WaitTOF();
}

/* Non-blocking key read, shared with input.c via amigacon.h. */
int con_getkey(void) {
    struct IntuiMessage *msg;
    int result = -1;
    msg = (struct IntuiMessage *)GetMsg(win->UserPort);
    if (msg) {
        if (msg->Class == IDCMP_VANILLAKEY) result = (int)(unsigned char)msg->Code;
        ReplyMsg((struct Message *)msg);
    }
    return result;
}

/* ---- primitives ---- */

void gfx_clear(unsigned char color) {
    gfx_fill_rect(0, 0, GFX_W, GFX_H, color);
}

void gfx_fill_rect(int x, int y, int w, int h, unsigned char color) {
    if (w <= 0 || h <= 0) return;
    SetAPen(rp, color);
    RectFill(rp, x, y, x + w - 1, y + h - 1);    /* RectFill is inclusive */
}

void gfx_frame_rect(int x, int y, int w, int h, unsigned char color) {
    if (w < 2 || h < 2) return;
    SetAPen(rp, color);
    RectFill(rp, x, y, x + w - 1, y);            /* top */
    RectFill(rp, x, y + h - 1, x + w - 1, y + h - 1); /* bottom */
    RectFill(rp, x, y, x, y + h - 1);            /* left */
    RectFill(rp, x + w - 1, y, x + w - 1, y + h - 1); /* right */
}

void gfx_char(int x, int y, char ch, unsigned char color) {
    SetAPen(rp, color);
    Move(rp, x, y + FONT_BASE);
    Text(rp, &ch, 1);
}

void gfx_text(int x, int y, const char *s, unsigned char color) {
    ULONG n = 0;
    while (s[n]) n++;
    if (!n) return;
    SetAPen(rp, color);
    Move(rp, x, y + FONT_BASE);
    Text(rp, (STRPTR)s, n);
}

/* ---- card art ---- */

#define CARD_W GFX_CARD_W
#define CARD_H GFX_CARD_H

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

void gfx_card(int x, int y, unsigned char suit, unsigned char value) {
    unsigned char sc = suit_col[suit <= 4 ? suit : 4];
    char vc = value_char(value);

    gfx_fill_rect(x + 3, y + 3, CARD_W, CARD_H, GC_SHADOW);        /* drop shadow */
    gfx_fill_rect(x, y, CARD_W, CARD_H, sc);                       /* suit border */
    gfx_fill_rect(x + 3, y + 3, CARD_W - 6, CARD_H - 6, GC_WHITE); /* body */
    gfx_char(x + 4, y + 4, vc, sc);                               /* top-left value */
    gfx_char(x + CARD_W - 12, y + CARD_H - 12, vc, sc);          /* bottom-right */
    gfx_fill_rect(x + CARD_W / 2 - 8, y + CARD_H / 2 - 9, 16, 18, sc); /* centre badge */
    gfx_char(x + CARD_W / 2 - 4, y + CARD_H / 2 - 4, vc, GC_WHITE);
}

void gfx_card_back(int x, int y) {
    gfx_fill_rect(x + 3, y + 3, CARD_W, CARD_H, GC_SHADOW);
    gfx_fill_rect(x, y, CARD_W, CARD_H, GC_WHITE);
    gfx_fill_rect(x + 3, y + 3, CARD_W - 6, CARD_H - 6, GC_RED);
    gfx_fill_rect(x + CARD_W / 2 - 7, y + CARD_H / 2 - 6, 14, 12, GC_WHITE);
    gfx_char(x + CARD_W / 2 - 4, y + CARD_H / 2 - 4, 'U', GC_RED);
}

#define HAND_LIFT 10

static int hand_step(unsigned char count) {
    int s;
    if (count <= 1) return 0;
    s = (304 - CARD_W) / (count - 1);
    if (s > 20) s = 20;
    if (s < 10) s = 10;
    return s;
}

void gfx_hand(int hx, int hy, const unsigned char *suits,
              const unsigned char *values, unsigned char count, unsigned char selected) {
    unsigned char i;
    int step = hand_step(count);
    int sx;

    for (i = 0; i < count; i++) {
        if (i == selected) continue;
        gfx_card(hx + i * step, hy, suits[i], values[i]);
    }
    sx = hx + selected * step;
    gfx_card(sx, hy - HAND_LIFT, suits[selected], values[selected]);
    gfx_frame_rect(sx - 2, hy - HAND_LIFT - 2, CARD_W + 4, CARD_H + 4, GC_YELLOW);
    gfx_frame_rect(sx - 1, hy - HAND_LIFT - 1, CARD_W + 2, CARD_H + 2, GC_YELLOW);
}
