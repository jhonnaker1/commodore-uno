/* The Lisa UNO UI -- implements the shared ui.h interface on the 720x364
   1-bit screen, with the drawing primitives in lisagfx.c. It is the Mac
   port's UI (mac/src/ui.c) redrawn for the Lisa: the suits are told apart
   the way a black-and-white Mac does it, by fill pattern (red solid
   black, blue dark grey, green mid grey, yellow light grey), with the
   suit's letter in a white tab in the corner as a second cue; wild cards
   are quartered into all four, and the value sits in the white oval of a
   real UNO card.

   Lisa pixels are about 1.5 times as tall as they are wide, so shapes are
   drawn wider than they are tall to look right: a card is 64 x 66 pixels
   (about the shape of a real one on the screen), its corners 9 across and
   6 down, and text is drawn double width where it needs to be read
   easily. Positions are the top left of what they place. */
#include "game.h"
#include "cards.h"
#include "ui.h"
#include "lisagfx.h"
#include "libc.h"

#define CARD_W 64
#define CARD_H 66
#define CRX 9 /* corner radius, across and down */
#define CRY 6

/* layout */
#define OPP_Y 6
#define OPP_H 20
#define OPP_W 228
#define TABLE_Y 36
#define DRAW_X 170
#define TOP_X 260
#define LABEL_Y (TABLE_Y + CARD_H + 5)
#define INFO_X 380
#define MSG_TOP 120
#define MSG_BOT 158
#define MSG_Y1 122
#define MSG_Y2 140
#define HAND_X 12
#define HAND_STEP 70
#define HAND_Y1 172
#define HAND_Y2 252
#define HAND_LIFT 6

/* text: S = small (single width), U = the UI's (double width) */
#define S 1, 1
#define U 2, 1

static const char *const color_name[4] = {"Red", "Yellow", "Green", "Blue"};
static const char suit_letter[4] = {'R', 'Y', 'G', 'B'};

static unsigned char mode = UI_MODE_PRESS;

/* where the hand was last drawn, for hit-testing */
static unsigned char hand_first, hand_shown;

/* What each hand slot shows now, so ui_draw_hand() redraws only the slots
   that changed: a card takes a tenth of a second to draw on a Lisa, and
   every CPU turn asks for the hand again. Anything that clears the hand's
   part of the screen sets hand_valid to 0. */
#define SLOT_EMPTY 0xFF
static struct { unsigned char color, value, lifted; } slot[HAND_VISIBLE];
static unsigned char hand_valid, hand_arrows;
static void screen_cleared(void);


/* ---- small helpers ---- */

static unsigned char suit_pat(unsigned char suit) {
    switch (suit) {
    case COLOR_RED: return PAT_BLACK;
    case COLOR_YELLOW: return PAT_LTGRAY;
    case COLOR_GREEN: return PAT_GRAY;
    default: return PAT_DKGRAY; /* blue */
    }
}

static void text_centered(short cx, short y, const char *s, unsigned char sx,
                          unsigned char sy, unsigned char flags) {
    gfx_text((short)(cx - gfx_text_w(s, sx, flags) / 2), y, s, sx, sy, flags);
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

static void clear(short top, short bottom) {
    gfx_rect(0, top, GFX_W, (short)(bottom - top), PAT_WHITE);
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
    char lbl[4];

    gfx_round(x, y, CARD_W, CARD_H, CRX, CRY, PAT_BLACK);
    if (suit == COLOR_WILD)
        gfx_round4(x + 1, y + 1, CARD_W - 2, CARD_H - 2, CRX - 1, CRY - 1,
                   suit_pat(COLOR_RED), suit_pat(COLOR_YELLOW),
                   suit_pat(COLOR_GREEN), suit_pat(COLOR_BLUE));
    else
        gfx_round(x + 1, y + 1, CARD_W - 2, CARD_H - 2, CRX - 1, CRY - 1, suit_pat(suit));

    /* the white oval, framed */
    gfx_round(x + 7, y + 16, CARD_W - 14, CARD_H - 32, (CARD_W - 14) / 2, (CARD_H - 32) / 2, PAT_BLACK);
    gfx_round(x + 9, y + 17, CARD_W - 18, CARD_H - 34, (CARD_W - 18) / 2, (CARD_H - 34) / 2, PAT_WHITE);
    value_label(lbl, value);
    text_centered((short)(x + CARD_W / 2), (short)(y + CARD_H / 2 - 7), lbl, 2, 2, TXT_BOLD);

    if (suit != COLOR_WILD) {
        char s[2];
        gfx_rect(x + 4, y + 3, 11, 13, PAT_BLACK);
        gfx_rect(x + 5, y + 4, 9, 11, PAT_WHITE);
        s[0] = suit_letter[suit];
        s[1] = 0;
        gfx_text((short)(x + 7), (short)(y + 5), s, S, 0);
    }
}

static void card_back(short x, short y) {
    gfx_round(x, y, CARD_W, CARD_H, CRX, CRY, PAT_BLACK);
    gfx_round(x + 5, y + 18, CARD_W - 10, CARD_H - 36, (CARD_W - 10) / 2, (CARD_H - 36) / 2, PAT_WHITE);
    text_centered((short)(x + CARD_W / 2), (short)(y + CARD_H / 2 - 4), "UNO", 2, 1, TXT_BOLD);
}

/* ---- screens ---- */

void ui_title_screen(void) {
    clear(0, GFX_H);
    screen_cleared();
    text_centered(GFX_W / 2, 30, "U N O", 6, 4, TXT_BOLD);
    text_centered(GFX_W / 2, 74, "1 player vs 3 CPU players", U, 0);
    card_face(GFX_W / 2 - 110, 100, COLOR_RED, 5);
    card_face(GFX_W / 2 - 32, 100, COLOR_YELLOW, 7);
    card_face(GFX_W / 2 + 46, 100, COLOR_WILD, VAL_WILD);
    text_centered(GFX_W / 2, 196, "Click a card to play it, or click the deck to draw.", U, 0);
    text_centered(GFX_W / 2, 212, "Keys: , and . (keypad 4, 6) pick, Return plays, U draws.", U, 0);
    text_centered(GFX_W / 2, 228, "1-9, 0 and A-J play the card in that slot directly.", U, 0);
    text_centered(GFX_W / 2, 270, "Click or press Return to start", U, TXT_BOLD);
    mode = UI_MODE_PRESS;
}

void ui_draw_frame(void) {
    clear(0, GFX_H);
    screen_cleared();
    mode = UI_MODE_GAME;
}

/* What the opponents' boxes and the table show now, as for the hand
   below: each part is redrawn only when what it shows has changed. */
static unsigned char opp_count[4], opp_lit[4], opp_valid;
static unsigned char tab_color, tab_value, tab_top_color, tab_valid;
static signed char tab_dir;
static unsigned char tab_deck;

/* The whole screen was cleared: everything has to be drawn again. */
static void screen_cleared(void) {
    opp_valid = 0;
    tab_valid = 0;
    hand_valid = 0;
}

void ui_draw_opponents(GameState *g) {
    unsigned char opp;
    for (opp = 1; opp <= 3; opp++) {
        char line[24], n[6];
        short x = (short)(12 + (opp - 1) * (OPP_W + 6));
        unsigned char count = g->players[opp].count, lit = g->current_player == opp;
        if (opp_valid && opp_count[opp] == count && opp_lit[opp] == lit) continue;
        opp_count[opp] = count;
        opp_lit[opp] = lit;
        gfx_rect(x, 0, OPP_W, OPP_Y + OPP_H + 2, PAT_WHITE);
        gfx_round_frame(x, OPP_Y, OPP_W, OPP_H, 9, 6, 2);
        player_name(line, opp);
        strcat(line, ":  ");
        strcat(line, num(n, count));
        strcat(line, count == 1 ? " card" : " cards");
        text_centered((short)(x + OPP_W / 2), OPP_Y + 6, line, U, 0);
        if (lit) gfx_round_invert(x + 2, OPP_Y + 1, OPP_W - 4, OPP_H - 2, 7, 5);
    }
    opp_valid = 1;
}

void ui_draw_table(GameState *g) {
    char buf[24], n[6];
    if (!tab_valid) {
        clear(TABLE_Y - 4, MSG_TOP);
        card_back(DRAW_X, TABLE_Y);
        text_centered(TOP_X + CARD_W / 2, LABEL_Y, "Discard", S, 0);
        gfx_text(INFO_X, TABLE_Y + 6, "Colour to match:", U, 0);
        gfx_rect(INFO_X, TABLE_Y + 22, 34, 16, PAT_BLACK);
    }
    if (!tab_valid || tab_color != g->top_card.color || tab_value != g->top_card.value) {
        tab_color = g->top_card.color;
        tab_value = g->top_card.value;
        card_face(TOP_X, TABLE_Y, tab_color, tab_value);
    }
    if (!tab_valid || tab_deck != g->draw_count) {
        tab_deck = g->draw_count;
        gfx_rect(DRAW_X - 20, LABEL_Y, CARD_W + 40, 10, PAT_WHITE);
        strcpy(buf, "Deck (");
        strcat(buf, num(n, tab_deck));
        strcat(buf, ")");
        text_centered(DRAW_X + CARD_W / 2, LABEL_Y, buf, S, 0);
    }
    if (!tab_valid || tab_top_color != g->top_color) {
        tab_top_color = g->top_color;
        gfx_rect(INFO_X + 2, TABLE_Y + 23, 30, 14, suit_pat(tab_top_color));
        gfx_rect(INFO_X + 44, TABLE_Y + 26, GFX_W - INFO_X - 44, 10, PAT_WHITE);
        gfx_text(INFO_X + 44, TABLE_Y + 26, color_name[tab_top_color], U, TXT_BOLD);
    }
    if (!tab_valid || tab_dir != g->direction) {
        tab_dir = g->direction;
        gfx_rect(INFO_X, TABLE_Y + 50, GFX_W - INFO_X, 10, PAT_WHITE);
        gfx_text(INFO_X, TABLE_Y + 50,
                 tab_dir > 0 ? "Play order: forward" : "Play order: reversed", U, 0);
    }
    tab_valid = 1;
}

static void slot_pos(unsigned char i, short *x, short *y) {
    *x = (short)(HAND_X + (i % 10) * HAND_STEP);
    *y = (short)(i < 10 ? HAND_Y1 : HAND_Y2);
}

/* A slot's area: the card, lifted or not, and the frame round a lifted
   one. Next to each other, slots' areas overlap only in those frames. */
static void slot_clear(unsigned char i) {
    short x, y;
    slot_pos(i, &x, &y);
    gfx_rect(x - 5, y - HAND_LIFT - 4, CARD_W + 10, CARD_H + HAND_LIFT + 8, PAT_WHITE);
}

static void slot_draw(unsigned char i) {
    short x, y;
    slot_pos(i, &x, &y);
    if (slot[i].color == SLOT_EMPTY) return;
    if (slot[i].lifted) {
        y -= HAND_LIFT;
        gfx_round_frame(x - 5, y - 4, CARD_W + 10, CARD_H + 8, CRX + 4, CRY + 3, 3);
    }
    card_face(x, y, slot[i].color, slot[i].value);
}

void ui_draw_hand(GameState *g, unsigned char cursor) {
    Player *p = &g->players[0];
    unsigned char i, first = 0, n, arrows, lifted_slot = SLOT_EMPTY;
    unsigned char changed[HAND_VISIBLE];

    if (p->count > HAND_VISIBLE && cursor >= HAND_VISIBLE)
        first = (unsigned char)(cursor - (HAND_VISIBLE - 1));
    n = (unsigned char)(p->count - first);
    if (n > HAND_VISIBLE) n = HAND_VISIBLE;
    arrows = (unsigned char)((first > 0) | ((first + n < p->count) << 1));
    /* the arrows overlap the end slots: with them, it all goes again */
    if (hand_first != first || arrows) hand_valid = 0;
    hand_first = first;
    hand_shown = n;
    hand_arrows = arrows;

    /* what each slot should show, and which differ from what is there */
    for (i = 0; i < HAND_VISIBLE; i++) {
        unsigned char c = SLOT_EMPTY, v = 0, l = 0;
        if (i < n) {
            unsigned char idx = (unsigned char)(first + i);
            c = p->hand[idx].color;
            v = p->hand[idx].value;
            l = (unsigned char)(idx == cursor);
            if (l) lifted_slot = i;
        }
        changed[i] = (unsigned char)(!hand_valid || slot[i].color != c ||
                                     slot[i].value != v || slot[i].lifted != l);
        slot[i].color = c;
        slot[i].value = v;
        slot[i].lifted = l;
    }

    if (!hand_valid) {
        clear(MSG_BOT, GFX_H);
        for (i = 0; i < n; i++) slot_draw(i);
        if (arrows & 1) gfx_text(0, HAND_Y1 + 70, "<", U, TXT_BOLD);
        if (arrows & 2) gfx_text(GFX_W - 12, HAND_Y1 + 70, ">", U, TXT_BOLD);
        hand_valid = 1;
    } else {
        /* Clearing a slot can cut into the lifted card's frame next to
           it, so that one is redrawn too, and last. */
        if (lifted_slot != SLOT_EMPTY && !changed[lifted_slot] &&
            ((lifted_slot > 0 && changed[lifted_slot - 1]) ||
             (lifted_slot + 1 < HAND_VISIBLE && changed[lifted_slot + 1])))
            changed[lifted_slot] = 1;
        for (i = 0; i < HAND_VISIBLE; i++)
            if (changed[i]) slot_clear(i);
        for (i = 0; i < HAND_VISIBLE; i++)
            if (changed[i] && i != lifted_slot) slot_draw(i);
        if (lifted_slot != SLOT_EMPTY && changed[lifted_slot]) slot_draw(lifted_slot);
    }
    mode = UI_MODE_GAME;
}

void ui_message(const char *line1, const char *line2) {
    clear(MSG_TOP, MSG_BOT);
    if (line1) gfx_text(12, MSG_Y1, line1, U, TXT_BOLD);
    if (line2) gfx_text(12, MSG_Y2, line2, U, 0);
}

/* colour picker: four swatches on the second message line */
#define PICK_X(i) ((short)(12 + (i) * 150))
#define PICK_W 140

void ui_draw_color_picker(unsigned char selected) {
    unsigned char i;
    clear(MSG_TOP, MSG_BOT);
    gfx_text(12, MSG_Y1, "Choose a colour:", U, TXT_BOLD);
    for (i = 0; i < 4; i++) {
        short x = PICK_X(i);
        gfx_rect(x + 6, MSG_Y2 - 1, 30, 13, PAT_BLACK);
        gfx_rect(x + 8, MSG_Y2, 26, 11, suit_pat(i));
        gfx_text(x + 44, MSG_Y2 + 2, color_name[i], U, 0);
        if (i == selected) gfx_round_frame(x, MSG_Y2 - 4, PICK_W, 19, 9, 6, 2);
    }
    mode = UI_MODE_PICKER;
}

void ui_clear_color_picker(void) {
    clear(MSG_TOP, MSG_BOT);
    mode = UI_MODE_GAME;
}

void ui_game_over_screen(unsigned char human_won, unsigned char winner_idx) {
    char line[24];
    clear(0, GFX_H);
    screen_cleared();
    if (human_won) {
        text_centered(GFX_W / 2, 110, "You win!", 5, 3, TXT_BOLD);
        text_centered(GFX_W / 2, 160, "Great game, champion.", U, 0);
    } else {
        player_name(line, winner_idx);
        strcat(line, " wins");
        text_centered(GFX_W / 2, 110, line, 5, 3, TXT_BOLD);
        text_centered(GFX_W / 2, 160, "Better luck next hand.", U, 0);
    }
    text_centered(GFX_W / 2, 230, "Click or press Return to play again", U, TXT_BOLD);
    mode = UI_MODE_PRESS;
}

/* ---- events and prompts ---- */

#define YES_X 420
#define NO_X 520
#define BTN_W 84

static void button(short x, const char *label, unsigned char on) {
    gfx_round(x, MSG_Y2 - 4, BTN_W, 19, 9, 6, PAT_WHITE);
    gfx_round_frame(x, MSG_Y2 - 4, BTN_W, 19, 9, 6, 2);
    text_centered((short)(x + BTN_W / 2), MSG_Y2 + 2, label, U, TXT_BOLD);
    if (on) gfx_round_invert(x + 2, MSG_Y2 - 3, BTN_W - 4, 17, 7, 5);
}

void ui_draw_challenge_prompt(unsigned char victim, unsigned char player, unsigned char selected_yes) {
    char line[48];
    clear(MSG_TOP, MSG_BOT);
    player_name(line, player);
    strcat(line, " played a Wild Draw Four");
    gfx_text(12, MSG_Y1, line, U, TXT_BOLD);
    player_name(line, victim);
    strcat(line, victim == 0 ? ", challenge it?" : " may challenge");
    gfx_text(12, MSG_Y2 + 2, line, U, 0);
    button(YES_X, "Yes", selected_yes);
    button(NO_X, "No", (unsigned char)!selected_yes);
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
    ui_message(line, 0);
}

void ui_event_skip(unsigned char idx) {
    event1(idx, idx == 0 ? " lose a turn" : " is skipped");
}

void ui_event_reverse(unsigned char idx) {
    (void)idx;
    ui_message("Reverse! The play order flips.", 0);
}

void ui_event_draw(unsigned char idx, unsigned char count) {
    char rest[24], n[6];
    strcpy(rest, idx == 0 ? " must draw " : " draws ");
    strcat(rest, num(n, count));
    event1(idx, rest);
}

void ui_event_uno(unsigned char idx) {
    char line[48];
    clear(MSG_Y2 - 2, MSG_BOT);
    player_name(line, idx);
    strcat(line, idx == 0 ? " have UNO! One card left!" : " has UNO! One card left!");
    gfx_text(12, MSG_Y2, line, U, TXT_BOLD);
}

void ui_event_invalid(void) {
    clear(MSG_Y2 - 2, MSG_BOT);
    gfx_text(12, MSG_Y2, "That card doesn't match - pick another, or draw.", U, 0);
}

void ui_event_drew_one(unsigned char idx) {
    event1(idx, " drew a card");
}

void ui_event_thinking(unsigned char idx) {
    event1(idx, " is thinking...");
}

/* ---- hit-testing (for the mouse) ---- */

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
            if (in_rect(x, y, PICK_X(i), MSG_Y2 - 4, PICK_X(i) + PICK_W, MSG_Y2 + 15)) return i;
        return HIT_NONE;
    case UI_MODE_CHALLENGE:
        if (in_rect(x, y, YES_X, MSG_Y2 - 4, YES_X + BTN_W, MSG_Y2 + 15)) return 1;
        if (in_rect(x, y, NO_X, MSG_Y2 - 4, NO_X + BTN_W, MSG_Y2 + 15)) return 0;
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
