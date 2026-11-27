/* The UNO UI for the NeXT -- the Mac port's (mac/src/ui.c), the same
   layout and words, drawn with Display PostScript through nextgfx.h
   rather than QuickDraw.

   On a colour NeXT the suits are the real UNO colours; on a greyscale
   NeXTstation they are four greys (red black, blue dark, green light,
   yellow lightest), as the black-and-white Mac's four patterns. Either way
   the suit's letter sits in a white tab in the corner as a second cue,
   wild cards are quartered into all four, and the value sits in the white
   oval of a real UNO card. */
#include <string.h>
#include "game.h"
#include "cards.h"
#include "ui.h"
#include "nextgfx.h"

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

/* The real UNO colours, and the greys that stand in for them. */
static const float suit_rgb[4][3] = {
    {0.88, 0.08, 0.08}, /* red */
    {0.97, 0.81, 0.00}, /* yellow */
    {0.00, 0.63, 0.22}, /* green */
    {0.03, 0.27, 0.81}, /* blue */
};
static const float suit_grey[4] = {0.0, 0.85, 0.6, 0.33};

static void suit_color(unsigned char suit) {
    if (gfx_has_color()) gfx_rgb(suit_rgb[suit][0], suit_rgb[suit][1], suit_rgb[suit][2]);
    else gfx_gray(suit_grey[suit]);
}

static void black(void) { gfx_gray(0.0); }
static void white(void) { gfx_gray(1.0); }

static void font_ui(void) { gfx_font(FONT_UI); }
static void font_small(void) { gfx_font(FONT_SMALL); }

static void text_at(short x, short y, const char *s) {
    gfx_text(x, y, s);
}

static short text_w(const char *s) {
    return gfx_text_w(s);
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
    white();
    gfx_fill_rect(left, top, right, bottom);
    black();
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
    short r = x + CARD_W, b = y + CARD_H;
    char lbl[4];

    if (suit == COLOR_WILD) {
        short mx = x + CARD_W / 2, my = y + CARD_H / 2;
        gfx_clip_round(x, y, r, b, 5.0);
        suit_color(COLOR_RED); gfx_fill_rect(x, y, mx, my);
        suit_color(COLOR_YELLOW); gfx_fill_rect(mx, y, r, my);
        suit_color(COLOR_GREEN); gfx_fill_rect(x, my, mx, b);
        suit_color(COLOR_BLUE); gfx_fill_rect(mx, my, r, b);
        gfx_unclip();
    } else {
        suit_color(suit);
        gfx_fill_round(x, y, r, b, 5.0);
    }
    black();
    gfx_frame_round(x, y, r, b, 5.0, 1.0);

    /* the white oval, framed */
    gfx_fill_oval(x + 5, y + 13, r - 5, b - 13);
    white();
    gfx_fill_oval(x + 6, y + 14, r - 6, b - 14);
    black();
    value_label(lbl, value);
    gfx_font(FONT_BOLD);
    text_centered((short)(x + CARD_W / 2), (short)(y + CARD_H / 2 + 5), lbl);

    if (suit != COLOR_WILD) {
        char s[2];
        white();
        gfx_fill_rect(x + 3, y + 3, x + 13, y + 14);
        black();
        gfx_frame_rect(x + 3, y + 3, x + 13, y + 14);
        s[0] = suit_letter[suit];
        s[1] = 0;
        font_small();
        text_centered((short)(x + 8), (short)(y + 12), s);
    }
    font_ui();
}

static void card_back(short x, short y) {
    short r = x + CARD_W, b = y + CARD_H;
    black();
    gfx_fill_round(x, y, r, b, 5.0);
    gfx_font(FONT_BOLD);
    if (gfx_has_color()) {
        /* the real back: a red oval on black with yellow lettering */
        suit_color(COLOR_RED);
        gfx_fill_oval(x + 3, y + 15, r - 3, b - 15);
        suit_color(COLOR_YELLOW);
    } else {
        white();
        gfx_fill_oval(x + 3, y + 15, r - 3, b - 15);
        black();
    }
    text_centered((short)(x + CARD_W / 2), (short)(y + CARD_H / 2 + 4), "UNO");
    black();
    font_ui();
}

/* ---- screens ---- */

void ui_title_screen(void) {
    gfx_begin();
    clear(0, 0, GFX_H, GFX_W);
    gfx_font(FONT_BIG);
    text_centered(GFX_W / 2, 44, "U N O");
    font_ui();
    text_centered(GFX_W / 2, 70, "1 player vs 3 CPU players");
    card_face(176, 86, COLOR_RED, 5);
    card_face(228, 86, COLOR_YELLOW, 7);
    card_face(280, 86, COLOR_WILD, VAL_WILD);
    text_centered(GFX_W / 2, 176, "Click a card to play it, or click the deck to draw.");
    text_centered(GFX_W / 2, 194, "Keys: arrows (or , and .) pick, Return plays, U draws.");
    text_centered(GFX_W / 2, 212, "1-9, 0 and A-J play a card in that slot directly.");
    gfx_font(FONT_BOLD);
    text_centered(GFX_W / 2, 250, "Click or press Return to start");
    gfx_end();
    mode = UI_MODE_PRESS;
}

void ui_draw_frame(void) {
    gfx_begin();
    clear(0, 0, GFX_H, GFX_W);
    gfx_end();
    mode = UI_MODE_GAME;
}

void ui_draw_opponents(GameState *g) {
    unsigned char opp;
    gfx_begin();
    clear(0, 0, OPP_Y + OPP_H + 2, GFX_W);
    font_ui();
    for (opp = 1; opp <= 3; opp++) {
        char line[24], n[6];
        short x = (short)(8 + (opp - 1) * 162);
        unsigned char lit = g->current_player == opp;
        if (lit) gfx_fill_round(x, OPP_Y, x + 154, OPP_Y + OPP_H, 4.0);
        else gfx_frame_round(x, OPP_Y, x + 154, OPP_Y + OPP_H, 4.0, 1.0);
        player_name(line, opp);
        strcat(line, ":  ");
        strcat(line, num(n, g->players[opp].count));
        strcat(line, g->players[opp].count == 1 ? " card" : " cards");
        if (lit) white();
        text_centered((short)(x + 77), OPP_Y + 15, line);
        black();
    }
    gfx_end();
}

void ui_draw_table(GameState *g) {
    char buf[24], n[6];
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
    suit_color(g->top_color);
    gfx_fill_rect(INFO_X, TABLE_Y + 22, INFO_X + 22, TABLE_Y + 36);
    black();
    gfx_frame_rect(INFO_X, TABLE_Y + 22, INFO_X + 22, TABLE_Y + 36);
    text_at(INFO_X + 30, TABLE_Y + 34, color_name[g->top_color]);
    text_at(INFO_X, TABLE_Y + 58,
            g->direction > 0 ? "Play order: forward" : "Play order: reversed");
    gfx_end();
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
            y -= HAND_LIFT;
            card_face(x, y, p->hand[idx].color, p->hand[idx].value);
            gfx_frame_round(x - 3, y - 3, x + CARD_W + 3, y + CARD_H + 3, 7.0, 2.0);
        } else {
            card_face(x, y, p->hand[idx].color, p->hand[idx].value);
        }
    }
    /* more cards than fit: arrows at the sides say which way they are */
    font_ui();
    if (first > 0) text_at(1, HAND_Y1 + 64, "<");
    if (first + n < p->count) text_at(GFX_W - 9, HAND_Y1 + 64, ">");
    gfx_end();
    mode = UI_MODE_GAME;
}

void ui_message(const char *line1, const char *line2) {
    gfx_begin();
    clear(MSG_TOP, 0, MSG_BOT, GFX_W);
    font_ui();
    if (line1) { gfx_font(FONT_BOLD); text_at(8, MSG_Y1, line1); font_ui(); }
    if (line2) text_at(8, MSG_Y2, line2);
    gfx_end();
}

/* colour picker: four swatches on the second message line */
#define PICK_X(i) ((short)(8 + (i) * 100))

void ui_draw_color_picker(unsigned char selected) {
    unsigned char i;
    gfx_begin();
    clear(MSG_TOP, 0, MSG_BOT, GFX_W);
    gfx_font(FONT_BOLD);
    text_at(8, MSG_Y1, "Choose a colour:");
    font_ui();
    for (i = 0; i < 4; i++) {
        short x = PICK_X(i);
        suit_color(i);
        gfx_fill_rect(x + 3, MSG_Y2 - 11, x + 23, MSG_Y2 + 1);
        black();
        gfx_frame_rect(x + 3, MSG_Y2 - 11, x + 23, MSG_Y2 + 1);
        text_at(x + 28, MSG_Y2, color_name[i]);
        if (i == selected) gfx_frame_round(x, MSG_Y2 - 14, x + 90, MSG_Y2 + 4, 4.0, 2.0);
    }
    gfx_end();
    mode = UI_MODE_PICKER;
}

void ui_clear_color_picker(void) {
    gfx_begin();
    clear(MSG_TOP, 0, MSG_BOT, GFX_W);
    gfx_end();
    mode = UI_MODE_GAME;
}

void ui_game_over_screen(unsigned char human_won, unsigned char winner_idx) {
    char line[24];
    gfx_begin();
    clear(0, 0, GFX_H, GFX_W);
    gfx_font(FONT_BIG);
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
    gfx_font(FONT_BOLD);
    text_centered(GFX_W / 2, 200, "Click or press Return to play again");
    gfx_end();
    mode = UI_MODE_PRESS;
}

/* ---- events and prompts ---- */

#define YES_X 200
#define NO_X 270
#define BTN_W 56

static void button(short x, const char *label, unsigned char on) {
    if (on) {
        gfx_fill_round(x, MSG_Y2 - 13, x + BTN_W, MSG_Y2 + 4, 5.0);
        white();
    } else {
        white();
        gfx_fill_round(x, MSG_Y2 - 13, x + BTN_W, MSG_Y2 + 4, 5.0);
        black();
        gfx_frame_round(x, MSG_Y2 - 13, x + BTN_W, MSG_Y2 + 4, 5.0, 1.0);
    }
    text_centered((short)(x + BTN_W / 2), MSG_Y2, label);
    black();
}

void ui_draw_challenge_prompt(unsigned char victim, unsigned char player, unsigned char selected_yes) {
    char line[48];
    gfx_begin();
    clear(MSG_TOP, 0, MSG_BOT, GFX_W);
    font_ui();
    player_name(line, player);
    strcat(line, " played a Wild Draw Four");
    gfx_font(FONT_BOLD);
    text_at(8, MSG_Y1, line);
    font_ui();
    player_name(line, victim);
    strcat(line, victim == 0 ? ", challenge it?" : " may challenge");
    text_at(8, MSG_Y2, line);
    button(YES_X, "Yes", selected_yes);
    button(NO_X, "No", (unsigned char)!selected_yes);
    gfx_end();
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
    gfx_font(FONT_BOLD);
    player_name(line, idx);
    strcat(line, idx == 0 ? " have UNO! One card left!" : " has UNO! One card left!");
    text_at(8, MSG_Y2, line);
    gfx_end();
}

void ui_event_invalid(void) {
    gfx_begin();
    clear(MSG_Y2 - 13, 0, MSG_BOT, GFX_W);
    font_ui();
    text_at(8, MSG_Y2, "That card doesn't match - pick another, or draw.");
    gfx_end();
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
