/* QuickDraw UNO UI for the classic Mac -- implements the shared ui.h
   interface, drawing into macgfx.c's offscreen buffer.

   On a colour Mac (macgfx.c decides) the suits are the real UNO colours.
   Otherwise it is one bit per pixel, and the suits are told apart the way
   a black-and-white Mac would: by fill pattern (red solid black, blue dark
   grey, green mid grey, yellow light grey). Either way the suit's letter
   sits in a white tab in the corner as a second cue, wild cards are
   quartered into all four, and the value sits in the white oval of a real
   UNO card. */
#include <Quickdraw.h>
#include <Fonts.h>
#include <string.h>
#include "game.h"
#include "cards.h"
#include "ui.h"
#include "macgfx.h"

#define CARD_W 40
#define CARD_H 58

/* layout of the 496x296 window */
#define OPP_Y 6
#define OPP_H 20
#define TABLE_Y 38
#define DRAW_X 56
#define TOP_X 128
#define LABEL_Y 112
#define INFO_X 212
#define MSG_TOP 116
#define MSG_BOT 152
#define MSG_Y1 131
#define MSG_Y2 147
#define HAND_X 12
#define HAND_STEP 48
#define HAND_Y1 162
#define HAND_Y2 230
#define HAND_LIFT 5

static const char *const color_name[4] = {"Red", "Yellow", "Green", "Blue"};
static const char suit_letter[4] = {'R', 'Y', 'G', 'B'};

static unsigned char mode = UI_MODE_PRESS;

/* where the hand was last drawn, for hit-testing */
static unsigned char hand_first, hand_shown;

/* ---- small helpers ---- */

static const Pattern *suit_pat(unsigned char suit) {
    switch (suit) {
    case COLOR_RED: return &qd.black;
    case COLOR_YELLOW: return &qd.ltGray;
    case COLOR_GREEN: return &qd.gray;
    default: return &qd.dkGray; /* blue */
    }
}

/* The real UNO colours, used when macgfx.c is drawing in colour. */
static const RGBColor suit_rgb[4] = {
    {0xE000, 0x1400, 0x1400}, /* red */
    {0xF800, 0xD000, 0x0000}, /* yellow */
    {0x0000, 0xA000, 0x3800}, /* green */
    {0x0800, 0x4400, 0xD000}, /* blue */
};

/* Fill with a suit: its colour, or on a 1-bit screen its pattern. */
static void suit_rect(const Rect *r, unsigned char suit) {
    if (gfx_has_color()) {
        RGBForeColor((RGBColor *)&suit_rgb[suit]);
        PaintRect(r);
        ForeColor(blackColor);
    } else {
        FillRect(r, suit_pat(suit));
    }
}

static void suit_round(const Rect *r, unsigned char suit) {
    if (gfx_has_color()) {
        RGBForeColor((RGBColor *)&suit_rgb[suit]);
        PaintRoundRect(r, 10, 10);
        ForeColor(blackColor);
    } else {
        FillRoundRect(r, 10, 10, suit_pat(suit));
    }
}

static void font_ui(void) { TextFont(systemFont); TextSize(12); TextFace(0); }
#define FONT_GENEVA 3 /* font family number; fixed since the 128K */
static void font_small(void) { TextFont(FONT_GENEVA); TextSize(9); TextFace(0); }

static void text_at(short x, short y, const char *s) {
    MoveTo(x, y);
    DrawText((Ptr)s, 0, (short)strlen(s));
}

static short text_w(const char *s) {
    return TextWidth((Ptr)s, 0, (short)strlen(s));
}

static void text_centered(short cx, short y, const char *s) {
    text_at((short)(cx - text_w(s) / 2), y, s);
}

/* unsigned to decimal, into buf (>= 6 bytes) */
static char *num(char *buf, unsigned int n) {
    char tmp[6];
    unsigned char i = 0, j = 0;
    do { tmp[i++] = (char)('0' + n % 10); n /= 10; } while (n && i < 5);
    while (i) buf[j++] = tmp[--i];
    buf[j] = 0;
    return buf;
}

static void player_name(char *out, unsigned char idx) {
    if (idx == 0) { strcpy(out, "You"); return; }
    strcpy(out, "CPU ");
    out[4] = (char)('0' + idx);
    out[5] = 0;
}

static void clear(short top, short left, short bottom, short right) {
    Rect r;
    SetRect(&r, left, top, right, bottom);
    EraseRect(&r);
}

/* ---- cards ---- */

static void value_label(char *out, unsigned char v) {
    if (v <= 9) { out[0] = (char)('0' + v); out[1] = 0; return; }
    switch (v) {
    case VAL_SKIP: strcpy(out, "S"); break;
    case VAL_REVERSE: strcpy(out, "R"); break;
    case VAL_DRAW2: strcpy(out, "+2"); break;
    case VAL_WILD: strcpy(out, "W"); break;
    default: strcpy(out, "+4"); break;
    }
}

static void card_face(short x, short y, unsigned char suit, unsigned char value) {
    Rect r, q, oval, tab;
    char lbl[4];

    SetRect(&r, x, y, x + CARD_W, y + CARD_H);
    PenNormal();
    if (suit == COLOR_WILD) {
        short mx = x + CARD_W / 2, my = y + CARD_H / 2;
        EraseRoundRect(&r, 10, 10);
        SetRect(&q, x, y, mx, my); suit_rect(&q, COLOR_RED);
        SetRect(&q, mx, y, x + CARD_W, my); suit_rect(&q, COLOR_YELLOW);
        SetRect(&q, x, my, mx, y + CARD_H); suit_rect(&q, COLOR_GREEN);
        SetRect(&q, mx, my, x + CARD_W, y + CARD_H); suit_rect(&q, COLOR_BLUE);
        /* the quadrant fills are square-cornered: trim them back to the
           card's rounded outline by erasing outside it */
        {
            RgnHandle card = NewRgn(), box = NewRgn();
            OpenRgn(); FrameRoundRect(&r, 10, 10); CloseRgn(card);
            RectRgn(box, &r);
            DiffRgn(box, card, box);
            EraseRgn(box);
            DisposeRgn(card);
            DisposeRgn(box);
        }
    } else {
        suit_round(&r, suit);
    }
    FrameRoundRect(&r, 10, 10);

    SetRect(&oval, x + 5, y + 13, x + CARD_W - 5, y + CARD_H - 13);
    EraseOval(&oval);
    FrameOval(&oval);
    value_label(lbl, value);
    font_ui();
    text_centered((short)(x + CARD_W / 2), (short)(y + CARD_H / 2 + 5), lbl);

    if (suit != COLOR_WILD) {
        char s[2];
        SetRect(&tab, x + 3, y + 3, x + 13, y + 14);
        EraseRect(&tab);
        FrameRect(&tab);
        s[0] = suit_letter[suit];
        s[1] = 0;
        font_small();
        text_at((short)(x + 5), (short)(y + 12), s);
    }
}

static void card_back(short x, short y) {
    Rect r, oval;
    SetRect(&r, x, y, x + CARD_W, y + CARD_H);
    PenNormal();
    FillRoundRect(&r, 10, 10, &qd.black);
    SetRect(&oval, x + 3, y + 15, x + CARD_W - 3, y + CARD_H - 15);
    font_ui();
    if (gfx_has_color()) {
        /* the real back: a red oval on black with yellow lettering */
        RGBForeColor((RGBColor *)&suit_rgb[COLOR_RED]);
        PaintOval(&oval);
        RGBForeColor((RGBColor *)&suit_rgb[COLOR_YELLOW]);
        text_centered((short)(x + CARD_W / 2), (short)(y + CARD_H / 2 + 4), "UNO");
        ForeColor(blackColor);
        return;
    }
    EraseOval(&oval);
    text_centered((short)(x + CARD_W / 2), (short)(y + CARD_H / 2 + 4), "UNO");
}

/* ---- screens ---- */

void ui_title_screen(void) {
    Rect all;
    gfx_begin();
    SetRect(&all, 0, 0, GFX_W, GFX_H);
    EraseRect(&all);
    TextFont(systemFont); TextSize(24); TextFace(bold);
    text_centered(GFX_W / 2, 44, "U N O");
    font_ui();
    text_centered(GFX_W / 2, 70, "1 player vs 3 CPU players");
    card_face(176, 86, COLOR_RED, 5);
    card_face(228, 86, COLOR_YELLOW, 7);
    card_face(280, 86, COLOR_WILD, VAL_WILD);
    text_centered(GFX_W / 2, 176, "Click a card to play it, or click the deck to draw.");
    text_centered(GFX_W / 2, 194, "Keys: arrows (or , and .) pick, Return plays, U draws.");
    text_centered(GFX_W / 2, 212, "1-9, 0 and A-J play a card in that slot directly.");
    TextFace(bold);
    text_centered(GFX_W / 2, 250, "Click or press Return to start");
    gfx_end(0, GFX_H);
    mode = UI_MODE_PRESS;
}

void ui_draw_frame(void) {
    Rect all;
    gfx_begin();
    SetRect(&all, 0, 0, GFX_W, GFX_H);
    EraseRect(&all);
    gfx_end(0, GFX_H);
    mode = UI_MODE_GAME;
}

void ui_draw_opponents(GameState *g) {
    unsigned char opp;
    gfx_begin();
    clear(0, 0, OPP_Y + OPP_H + 2, GFX_W);
    font_ui();
    for (opp = 1; opp <= 3; opp++) {
        Rect box;
        char line[24], n[6];
        short x = (short)(8 + (opp - 1) * 162);
        SetRect(&box, x, OPP_Y, x + 154, OPP_Y + OPP_H);
        FrameRoundRect(&box, 8, 8);
        player_name(line, opp);
        strcat(line, ":  ");
        strcat(line, num(n, g->players[opp].count));
        strcat(line, g->players[opp].count == 1 ? " card" : " cards");
        text_centered((short)(x + 77), OPP_Y + 15, line);
        if (g->current_player == opp) {
            InsetRect(&box, 1, 1);
            InvertRoundRect(&box, 8, 8);
        }
    }
    gfx_end(0, OPP_Y + OPP_H + 2);
}

void ui_draw_table(GameState *g) {
    char buf[24], n[6];
    Rect sw;
    gfx_begin();
    clear(TABLE_Y - 4, 0, MSG_TOP, GFX_W);

    card_back(DRAW_X, TABLE_Y);
    card_face(TOP_X, TABLE_Y, g->top_card.color, g->top_card.value);

    font_small();
    strcpy(buf, "Deck (");
    strcat(buf, num(n, g->draw_count));
    strcat(buf, ")");
    text_centered(DRAW_X + CARD_W / 2, LABEL_Y - 6, buf);
    text_centered(TOP_X + CARD_W / 2, LABEL_Y - 6, "Discard");

    font_ui();
    text_at(INFO_X, TABLE_Y + 14, "Colour to match:");
    SetRect(&sw, INFO_X, TABLE_Y + 22, INFO_X + 22, TABLE_Y + 36);
    suit_rect(&sw, g->top_color);
    FrameRect(&sw);
    text_at(INFO_X + 30, TABLE_Y + 34, color_name[g->top_color]);
    text_at(INFO_X, TABLE_Y + 58,
            g->direction > 0 ? "Play order: forward" : "Play order: reversed");
    gfx_end(TABLE_Y - 4, MSG_TOP);
}

void ui_draw_hand(GameState *g, unsigned char cursor) {
    Player *p = &g->players[0];
    unsigned char i, first = 0, n;

    if (p->count > HAND_VISIBLE && cursor >= HAND_VISIBLE)
        first = (unsigned char)(cursor - (HAND_VISIBLE - 1));
    n = (unsigned char)(p->count - first);
    if (n > HAND_VISIBLE) n = HAND_VISIBLE;
    hand_first = first;
    hand_shown = n;

    gfx_begin();
    clear(MSG_BOT, 0, GFX_H, GFX_W);
    for (i = 0; i < n; i++) {
        unsigned char idx = (unsigned char)(first + i);
        short x = (short)(HAND_X + (i % 10) * HAND_STEP);
        short y = (short)(i < 10 ? HAND_Y1 : HAND_Y2);
        if (idx == cursor) {
            Rect sel;
            y -= HAND_LIFT;
            card_face(x, y, p->hand[idx].color, p->hand[idx].value);
            SetRect(&sel, x - 3, y - 3, x + CARD_W + 3, y + CARD_H + 3);
            PenSize(2, 2);
            FrameRoundRect(&sel, 14, 14);
            PenNormal();
        } else {
            card_face(x, y, p->hand[idx].color, p->hand[idx].value);
        }
    }
    /* more cards than fit: arrows at the sides say which way they are */
    font_ui();
    if (first > 0) text_at(1, HAND_Y1 + 64, "<");
    if (first + n < p->count) text_at(GFX_W - 9, HAND_Y1 + 64, ">");
    gfx_end(MSG_BOT, GFX_H);
    mode = UI_MODE_GAME;
}

void ui_message(const char *line1, const char *line2) {
    gfx_begin();
    clear(MSG_TOP, 0, MSG_BOT, GFX_W);
    font_ui();
    if (line1) { TextFace(bold); text_at(8, MSG_Y1, line1); TextFace(0); }
    if (line2) text_at(8, MSG_Y2, line2);
    gfx_end(MSG_TOP, MSG_BOT);
}

/* colour picker: four swatches on the second message line */
#define PICK_X(i) ((short)(8 + (i) * 100))

void ui_draw_color_picker(unsigned char selected) {
    unsigned char i;
    gfx_begin();
    clear(MSG_TOP, 0, MSG_BOT, GFX_W);
    font_ui();
    TextFace(bold);
    text_at(8, MSG_Y1, "Choose a colour:");
    TextFace(0);
    for (i = 0; i < 4; i++) {
        Rect sw, frame;
        short x = PICK_X(i);
        SetRect(&sw, x + 3, MSG_Y2 - 11, x + 23, MSG_Y2 + 1);
        suit_rect(&sw, i);
        FrameRect(&sw);
        text_at(x + 28, MSG_Y2, color_name[i]);
        SetRect(&frame, x, MSG_Y2 - 14, x + 90, MSG_Y2 + 4);
        if (i == selected) { PenSize(2, 2); FrameRoundRect(&frame, 8, 8); PenNormal(); }
    }
    gfx_end(MSG_TOP, MSG_BOT);
    mode = UI_MODE_PICKER;
}

void ui_clear_color_picker(void) {
    gfx_begin();
    clear(MSG_TOP, 0, MSG_BOT, GFX_W);
    gfx_end(MSG_TOP, MSG_BOT);
    mode = UI_MODE_GAME;
}

void ui_game_over_screen(unsigned char human_won, unsigned char winner_idx) {
    Rect all;
    char line[24];
    gfx_begin();
    SetRect(&all, 0, 0, GFX_W, GFX_H);
    EraseRect(&all);
    TextFont(systemFont); TextSize(24); TextFace(bold);
    if (human_won) {
        text_centered(GFX_W / 2, 110, "You win!");
        font_ui();
        text_centered(GFX_W / 2, 140, "Great game, champion.");
    } else {
        player_name(line, winner_idx);
        strcat(line, " wins");
        text_centered(GFX_W / 2, 110, line);
        font_ui();
        text_centered(GFX_W / 2, 140, "Better luck next hand.");
    }
    TextFace(bold);
    text_centered(GFX_W / 2, 200, "Click or press Return to play again");
    gfx_end(0, GFX_H);
    mode = UI_MODE_PRESS;
}

/* ---- events and prompts ---- */

#define YES_X 200
#define NO_X 270
#define BTN_W 56

static void button(short x, const char *label, unsigned char on) {
    Rect b;
    SetRect(&b, x, MSG_Y2 - 13, x + BTN_W, MSG_Y2 + 4);
    EraseRoundRect(&b, 10, 10);
    FrameRoundRect(&b, 10, 10);
    text_centered((short)(x + BTN_W / 2), MSG_Y2, label);
    if (on) { InsetRect(&b, 1, 1); InvertRoundRect(&b, 10, 10); }
}

void ui_draw_challenge_prompt(unsigned char victim, unsigned char player, unsigned char selected_yes) {
    char line[48];
    gfx_begin();
    clear(MSG_TOP, 0, MSG_BOT, GFX_W);
    font_ui();
    player_name(line, player);
    strcat(line, " played a Wild Draw Four");
    TextFace(bold);
    text_at(8, MSG_Y1, line);
    TextFace(0);
    player_name(line, victim);
    strcat(line, victim == 0 ? ", challenge it?" : " may challenge");
    text_at(8, MSG_Y2, line);
    button(YES_X, "Yes", selected_yes);
    button(NO_X, "No", (unsigned char)!selected_yes);
    gfx_end(MSG_TOP, MSG_BOT);
    mode = UI_MODE_CHALLENGE;
}

void ui_event_challenge_result(unsigned char victim, unsigned char player, unsigned char succeeded) {
    char a[48];
    char b[48];
    player_name(a, victim);
    strcat(a, " challenges!");
    if (succeeded) {
        player_name(b, player);
        strcat(b, " had a match and draws 4");
    } else {
        player_name(b, victim);
        strcat(b, " was wrong and draws 6");
    }
    mode = UI_MODE_GAME;
    ui_message(a, b);
}

static void event1(unsigned char idx, const char *rest) {
    char line[48];
    player_name(line, idx);
    strcat(line, rest);
    ui_message(line, NULL);
}

void ui_event_skip(unsigned char idx) {
    event1(idx, idx == 0 ? " lose a turn" : " is skipped");
}

void ui_event_reverse(unsigned char idx) {
    (void)idx;
    ui_message("Reverse! The play order flips.", NULL);
}

void ui_event_draw(unsigned char idx, unsigned char count) {
    char rest[24], n[6];
    strcpy(rest, idx == 0 ? " must draw " : " draws ");
    strcat(rest, num(n, count));
    event1(idx, rest);
}

void ui_event_uno(unsigned char idx) {
    char line[48];
    gfx_begin();
    clear(MSG_Y2 - 13, 0, MSG_BOT, GFX_W);
    font_ui();
    TextFace(bold);
    player_name(line, idx);
    strcat(line, idx == 0 ? " have UNO! One card left!" : " has UNO! One card left!");
    text_at(8, MSG_Y2, line);
    gfx_end(MSG_Y2 - 13, MSG_BOT);
}

void ui_event_invalid(void) {
    gfx_begin();
    clear(MSG_Y2 - 13, 0, MSG_BOT, GFX_W);
    font_ui();
    text_at(8, MSG_Y2, "That card doesn't match - pick another, or draw.");
    gfx_end(MSG_Y2 - 13, MSG_BOT);
}

void ui_event_drew_one(unsigned char idx) {
    event1(idx, " drew a card");
}

void ui_event_thinking(unsigned char idx) {
    event1(idx, " is thinking...");
}

/* ---- hit-testing ---- */

unsigned char ui_mode(void) {
    return mode;
}

static unsigned char in_rect(short x, short y, short l, short t, short r, short b) {
    return (unsigned char)(x >= l && x < r && y >= t && y < b);
}

unsigned char ui_hit(short x, short y) {
    unsigned char i;
    switch (mode) {
    case UI_MODE_PRESS:
        return HIT_FIRE;
    case UI_MODE_PICKER:
        for (i = 0; i < 4; i++)
            if (in_rect(x, y, PICK_X(i), MSG_Y2 - 14, PICK_X(i) + 90, MSG_Y2 + 4)) return i;
        return HIT_NONE;
    case UI_MODE_CHALLENGE:
        if (in_rect(x, y, YES_X, MSG_Y2 - 13, YES_X + BTN_W, MSG_Y2 + 4)) return 1;
        if (in_rect(x, y, NO_X, MSG_Y2 - 13, NO_X + BTN_W, MSG_Y2 + 4)) return 0;
        return HIT_NONE;
    default:
        break;
    }
    if (in_rect(x, y, DRAW_X, TABLE_Y, DRAW_X + CARD_W, TABLE_Y + CARD_H)) return HIT_DRAW;
    for (i = 0; i < hand_shown; i++) {
        short cx = (short)(HAND_X + (i % 10) * HAND_STEP);
        short cy = (short)(i < 10 ? HAND_Y1 : HAND_Y2);
        /* the lifted card sits HAND_LIFT higher; accept the union */
        if (in_rect(x, y, cx, cy - HAND_LIFT, cx + CARD_W, cy + CARD_H))
            return (unsigned char)(hand_first + i);
    }
    return HIT_NONE;
}
