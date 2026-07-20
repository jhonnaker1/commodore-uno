/* UNO UI for the Atari ST/STE -- implements the shared ui.h interface,
   rendering pixel-art cards into the 320x200 16-colour planar framebuffer
   via stevid.c. Layout mirrors the VBXE/X16 bitmap builds, with the extra
   8 scanlines the ST has over the VBXE mode given to the hand. */
#include <string.h>
#include "game.h"
#include "cards.h"
#include "ui.h"
#include "stevid.h"

/* pixel layout for the 320x200 framebuffer */
#define TITLE_X 132
#define TITLE_Y 2
#define OPP_Y 14
#define TBL_LABEL_Y 26
#define CARD_Y 36
#define DRAW_X 8
#define TOP_X 88
#define INFO_X 150
#define MSG_Y1 94
#define MSG_Y2 104
#define HAND_TOP 116
#define HAND_Y 134

/* card color (0-4) -> palette index */
static const unsigned char suit_sc[5] = {SC_RED, SC_YELLOW, SC_GREEN, SC_BLUE, SC_GRAY};
static const char *const color_name[4] = {"RED", "YELLOW", "GREEN", "BLUE"};

static void palette_init(void) {
    ste_palette(SC_BLACK, 0, 0, 0);
    ste_palette(SC_WHITE, 15, 15, 15);
    ste_palette(SC_RED, 13, 2, 2);
    ste_palette(SC_GREEN, 2, 11, 4);
    ste_palette(SC_BLUE, 4, 6, 14);
    ste_palette(SC_YELLOW, 14, 12, 2);
    ste_palette(SC_GRAY, 9, 9, 9);
    ste_palette(SC_SHADOW, 1, 3, 2);
    ste_palette(SC_FELT, 1, 6, 3);
}

static void put_num(unsigned int x, unsigned int y, unsigned int n, unsigned char col) {
    char buf[6];
    unsigned char i = 0, j;
    if (n == 0) { ste_char(x, y, '0', col); return; }
    while (n && i < 5) { buf[i++] = (char)('0' + (n % 10)); n /= 10; }
    for (j = 0; j < i; j++) ste_char(x + (unsigned int)j * 8, y, (unsigned char)buf[i - 1 - j], col);
}

static void clear_area(unsigned int x, unsigned int y, unsigned int w, unsigned int h) {
    ste_fill_rect(x, y, w, h, SC_FELT);
}

static void text(unsigned int x, unsigned int y, const char *s, unsigned char col) {
    ste_text(x, y, s, col);
}

/* Draw the player's label ("YOU" or "CPUn") and return the x for the text
   that follows, leaving a one-character gap. */
static unsigned int player_label(unsigned int x, unsigned int y, unsigned char idx, unsigned char color) {
    if (idx == 0) { text(x, y, "YOU", color); return x + 4 * 8; }
    text(x, y, "CPU", color);
    ste_char(x + 24, y, (unsigned char)('0' + idx), color);
    return x + 5 * 8;
}

void ui_title_screen(void) {
    palette_init();
    ste_clear(SC_FELT);

    text(TITLE_X, 18, "U N O", SC_YELLOW);
    text(48, 36, ste_is_ste() ? "ATARI STE EDITION" : "ATARI ST EDITION", SC_WHITE);
    text(48, 58, "1 PLAYER VS 3 CPU PLAYERS", SC_GRAY);
    text(64, 80, "L/R - PICK A CARD", SC_WHITE);
    text(64, 92, "SPACE - PLAY A CARD", SC_WHITE);
    text(64, 104, "UP - DRAW A CARD", SC_WHITE);
    text(76, 126, "PRESS SPACE TO START", SC_YELLOW);
    ste_card(72, 146, 0, 5);
    ste_card(128, 146, 1, 7);
    ste_card(184, 146, 4, VAL_WILD);
}

void ui_draw_frame(void) {
    ste_clear(SC_FELT);
    text(TITLE_X, TITLE_Y, "U N O", SC_YELLOW);
}

void ui_draw_opponents(GameState *g) {
    unsigned char opp;
    clear_area(0, OPP_Y, 320, 8);
    for (opp = 1; opp <= 3; opp++) {
        unsigned int x = 8 + (unsigned int)(opp - 1) * 106;
        unsigned char col = (g->current_player == opp) ? SC_YELLOW : SC_WHITE;
        text(x, OPP_Y, "CPU", col);
        ste_char(x + 24, OPP_Y, (unsigned char)('0' + opp), col);
        ste_char(x + 32, OPP_Y, ':', col);
        put_num(x + 42, OPP_Y, g->players[opp].count, col);
    }
}

void ui_draw_table(GameState *g) {
    clear_area(0, TBL_LABEL_Y, 320, CARD_Y + STE_CARD_H + 6 - TBL_LABEL_Y);
    text(DRAW_X + 4, TBL_LABEL_Y, "DRAW", SC_GRAY);
    put_num(DRAW_X + 40, TBL_LABEL_Y, g->draw_count, SC_GRAY);
    ste_card_back(DRAW_X, CARD_Y);

    text(TOP_X, TBL_LABEL_Y, "TOP CARD", SC_GRAY);
    ste_card(TOP_X, CARD_Y, g->top_card.color, g->top_card.value);

    text(INFO_X, CARD_Y + 4, "COLOR", SC_WHITE);
    text(INFO_X, CARD_Y + 16, color_name[g->top_color], suit_sc[g->top_color]);
    text(INFO_X, CARD_Y + 34, g->direction > 0 ? "DIR ->" : "DIR <-", SC_WHITE);
}

void ui_draw_hand(GameState *g, unsigned char cursor) {
    static unsigned char hs[HAND_VISIBLE], hv[HAND_VISIBLE];
    Player *p = &g->players[0];
    unsigned char i, n = p->count > HAND_VISIBLE ? HAND_VISIBLE : p->count;

    clear_area(0, HAND_TOP, 320, BMP_H - HAND_TOP);
    for (i = 0; i < n; i++) {
        hs[i] = p->hand[i].color;
        hv[i] = p->hand[i].value;
    }
    if (n) ste_hand(6, HAND_Y, hs, hv, n, cursor < n ? cursor : 0);
}

void ui_message(const char *line1, const char *line2) {
    clear_area(0, MSG_Y1, 320, 18);
    if (line1) text(4, MSG_Y1, line1, SC_WHITE);
    if (line2) text(4, MSG_Y2, line2, SC_WHITE);
}

/* The selection frame is 12px tall at MSG_Y2-2, so it reaches 2px below the
   18px message band -- clear 20 so a moved selection leaves no bracket
   bottom behind. */
#define PICKER_CLR_H 20

void ui_draw_color_picker(unsigned char selected) {
    static const char *const names[4] = {"RED", "YELLOW", "GREEN", "BLUE"};
    unsigned char i;
    clear_area(0, MSG_Y1, 320, PICKER_CLR_H);
    text(4, MSG_Y1, "CHOOSE A COLOR:", SC_WHITE);
    for (i = 0; i < 4; i++) {
        unsigned int x = 4 + (unsigned int)i * 76;
        unsigned char c = suit_sc[i];
        /* width 70 clears the widest label ("YELLOW" ends at x+64) */
        if (i == selected) ste_frame_rect(x - 2, MSG_Y2 - 2, 70, 12, SC_YELLOW);
        ste_fill_rect(x, MSG_Y2, 12, 8, c);
        text(x + 16, MSG_Y2, names[i], c);
    }
}

void ui_clear_color_picker(void) {
    clear_area(0, MSG_Y1, 320, PICKER_CLR_H);
}

void ui_game_over_screen(unsigned char human_won, unsigned char winner_idx) {
    ste_clear(SC_FELT);
    if (human_won) {
        text(124, 72, "YOU WIN!", SC_YELLOW);
        text(72, 94, "GREAT GAME, CHAMPION.", SC_WHITE);
    } else {
        text(120, 72, "GAME OVER", SC_RED);
        text(112, 94, "CPU", SC_WHITE);
        ste_char(112 + 24, 94, (unsigned char)('0' + winner_idx), SC_WHITE);
        text(112 + 40, 94, "WINS", SC_WHITE);
    }
    text(68, 130, "PRESS SPACE TO PLAY AGAIN", SC_WHITE);
}

/* ---- event / prompt lines ---- */

void ui_draw_challenge_prompt(unsigned char victim, unsigned char player, unsigned char selected_yes) {
    unsigned int lx;
    clear_area(0, MSG_Y1, 320, 18);
    lx = player_label(4, MSG_Y1, player, SC_WHITE);
    text(lx, MSG_Y1, "PLAYED WILD DRAW FOUR", SC_WHITE);
    lx = player_label(4, MSG_Y2, victim, SC_WHITE);
    text(lx, MSG_Y2, "CHALLENGE?", SC_WHITE);
    text(124, MSG_Y2, "YES", selected_yes ? SC_YELLOW : SC_GRAY);
    text(164, MSG_Y2, "NO", selected_yes ? SC_GRAY : SC_YELLOW);
}

void ui_event_challenge_result(unsigned char victim, unsigned char player, unsigned char succeeded) {
    unsigned int lx;
    clear_area(0, MSG_Y1, 320, 18);
    lx = player_label(4, MSG_Y1, victim, SC_WHITE);
    text(lx, MSG_Y1, "CHALLENGES!", SC_WHITE);
    if (succeeded) {
        lx = player_label(4, MSG_Y2, player, SC_RED);
        text(lx, MSG_Y2, "HAD A MATCH - DRAWS 4", SC_RED);
    } else {
        lx = player_label(4, MSG_Y2, victim, SC_RED);
        text(lx, MSG_Y2, "WAS WRONG - DRAWS 6", SC_RED);
    }
}

void ui_event_skip(unsigned char idx) {
    unsigned int lx;
    clear_area(0, MSG_Y1, 320, 18);
    lx = player_label(4, MSG_Y1, idx, SC_WHITE);
    text(lx, MSG_Y1, idx == 0 ? "LOSE A TURN" : "IS SKIPPED", SC_WHITE);
}

void ui_event_reverse(unsigned char idx) {
    (void)idx;
    clear_area(0, MSG_Y1, 320, 18);
    text(4, MSG_Y1, "REVERSE! ORDER FLIPPED", SC_WHITE);
}

void ui_event_draw(unsigned char idx, unsigned char count) {
    const char *w = idx == 0 ? "MUST DRAW" : "DRAWS";
    unsigned int lx;
    clear_area(0, MSG_Y1, 320, 18);
    lx = player_label(4, MSG_Y1, idx, SC_WHITE);
    text(lx, MSG_Y1, w, SC_WHITE);
    put_num(lx + (unsigned int)strlen(w) * 8 + 8, MSG_Y1, count, SC_WHITE);
}

void ui_event_uno(unsigned char idx) {
    unsigned int lx;
    clear_area(0, MSG_Y2, 320, 8);
    lx = player_label(4, MSG_Y2, idx, SC_YELLOW);
    text(lx, MSG_Y2, "UNO! ONE CARD LEFT!", SC_YELLOW);
}

void ui_event_invalid(void) {
    clear_area(0, MSG_Y2, 320, 8);
    text(4, MSG_Y2, "THAT CARD DOES NOT MATCH!", SC_RED);
}

void ui_event_drew_one(unsigned char idx) {
    unsigned int lx;
    clear_area(0, MSG_Y1, 320, 18);
    lx = player_label(4, MSG_Y1, idx, SC_WHITE);
    text(lx, MSG_Y1, "DREW A CARD", SC_WHITE);
}

void ui_event_thinking(unsigned char idx) {
    unsigned int lx;
    clear_area(0, MSG_Y1, 320, 18);
    lx = player_label(4, MSG_Y1, idx, SC_WHITE);
    text(lx, MSG_Y1, "IS THINKING...", SC_WHITE);
}
