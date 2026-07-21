/* Bitmap UNO UI for the Amiga -- implements the shared ui.h interface,
   drawing pixel-art cards into the custom 320x256 16-colour screen via
   gfx.c. Same card/hand design as the Atari ST and X16/VBXE bitmap builds;
   the text build (ui.c + amigacon.c) is untouched. */
#include <string.h>
#include "game.h"
#include "cards.h"
#include "ui.h"
#include "gfx.h"

/* pixel layout for the 320x256 framebuffer */
#define TITLE_X 132
#define TITLE_Y 6
#define OPP_Y 22
#define TBL_LABEL_Y 40
#define CARD_Y 52
#define DRAW_X 8
#define TOP_X 88
#define INFO_X 150
#define MSG_Y1 120
#define MSG_Y2 132
#define HAND_TOP 150
#define HAND_Y 174

static const unsigned char suit_gc[5] = {GC_RED, GC_YELLOW, GC_GREEN, GC_BLUE, GC_GRAY};
static const char *const color_name[4] = {"RED", "YELLOW", "GREEN", "BLUE"};

static void put_num(int x, int y, unsigned int n, unsigned char col) {
    char buf[6];
    unsigned char i = 0, j;
    if (n == 0) { gfx_char(x, y, '0', col); return; }
    while (n && i < 5) { buf[i++] = (char)('0' + (n % 10)); n /= 10; }
    for (j = 0; j < i; j++) gfx_char(x + (int)j * 8, y, buf[i - 1 - j], col);
}

static void clear_area(int x, int y, int w, int h) {
    gfx_fill_rect(x, y, w, h, GC_FELT);
}

static void text(int x, int y, const char *s, unsigned char col) {
    gfx_text(x, y, s, col);
}

/* Draw the player's label ("YOU" or "CPUn") and return the x for the text
   after it, leaving a one-character gap. */
static int player_label(int x, int y, unsigned char idx, unsigned char color) {
    if (idx == 0) { text(x, y, "YOU", color); return x + 4 * 8; }
    text(x, y, "CPU", color);
    gfx_char(x + 24, y, (char)('0' + idx), color);
    return x + 5 * 8;
}

void ui_title_screen(void) {
    gfx_clear(GC_FELT);
    text(TITLE_X, 24, "U N O", GC_YELLOW);
    text(56, 44, "AMIGA BITMAP EDITION", GC_WHITE);
    text(48, 68, "1 PLAYER VS 3 CPU PLAYERS", GC_GRAY);
    text(56, 92, ", / . - PICK A CARD", GC_WHITE);
    text(56, 104, "SPACE - PLAY A CARD", GC_WHITE);
    text(56, 116, "U - DRAW A CARD", GC_WHITE);
    text(76, 140, "PRESS SPACE TO START", GC_YELLOW);
    gfx_card(72, 168, 0, 5);
    gfx_card(128, 168, 1, 7);
    gfx_card(184, 168, 4, VAL_WILD);
}

void ui_draw_frame(void) {
    gfx_clear(GC_FELT);
    text(TITLE_X, TITLE_Y, "U N O", GC_YELLOW);
}

void ui_draw_opponents(GameState *g) {
    unsigned char opp;
    clear_area(0, OPP_Y, 320, 8);
    for (opp = 1; opp <= 3; opp++) {
        int x = 8 + (int)(opp - 1) * 106;
        unsigned char col = (g->current_player == opp) ? GC_YELLOW : GC_WHITE;
        text(x, OPP_Y, "CPU", col);
        gfx_char(x + 24, OPP_Y, (char)('0' + opp), col);
        gfx_char(x + 32, OPP_Y, ':', col);
        put_num(x + 42, OPP_Y, g->players[opp].count, col);
    }
}

void ui_draw_table(GameState *g) {
    clear_area(0, TBL_LABEL_Y, 320, CARD_Y + GFX_CARD_H + 6 - TBL_LABEL_Y);
    text(DRAW_X + 4, TBL_LABEL_Y, "DRAW", GC_GRAY);
    put_num(DRAW_X + 40, TBL_LABEL_Y, g->draw_count, GC_GRAY);
    gfx_card_back(DRAW_X, CARD_Y);

    text(TOP_X, TBL_LABEL_Y, "TOP CARD", GC_GRAY);
    gfx_card(TOP_X, CARD_Y, g->top_card.color, g->top_card.value);

    text(INFO_X, CARD_Y + 4, "COLOR", GC_WHITE);
    text(INFO_X, CARD_Y + 16, color_name[g->top_color], suit_gc[g->top_color]);
    text(INFO_X, CARD_Y + 34, g->direction > 0 ? "DIR ->" : "DIR <-", GC_WHITE);
}

void ui_draw_hand(GameState *g, unsigned char cursor) {
    static unsigned char hs[HAND_VISIBLE], hv[HAND_VISIBLE];
    Player *p = &g->players[0];
    unsigned char i, n = p->count > HAND_VISIBLE ? HAND_VISIBLE : p->count;

    clear_area(0, HAND_TOP, 320, GFX_H - HAND_TOP);
    for (i = 0; i < n; i++) {
        hs[i] = p->hand[i].color;
        hv[i] = p->hand[i].value;
    }
    if (n) gfx_hand(6, HAND_Y, hs, hv, n, cursor < n ? cursor : 0);
}

void ui_message(const char *line1, const char *line2) {
    clear_area(0, MSG_Y1, 320, 18);
    if (line1) text(4, MSG_Y1, line1, GC_WHITE);
    if (line2) text(4, MSG_Y2, line2, GC_WHITE);
}

#define PICKER_CLR_H 20

void ui_draw_color_picker(unsigned char selected) {
    static const char *const names[4] = {"RED", "YELLOW", "GREEN", "BLUE"};
    unsigned char i;
    clear_area(0, MSG_Y1, 320, PICKER_CLR_H);
    text(4, MSG_Y1, "CHOOSE A COLOR:", GC_WHITE);
    for (i = 0; i < 4; i++) {
        int x = 4 + (int)i * 76;
        unsigned char c = suit_gc[i];
        if (i == selected) gfx_frame_rect(x - 2, MSG_Y2 - 2, 70, 12, GC_YELLOW);
        gfx_fill_rect(x, MSG_Y2, 12, 8, c);
        text(x + 16, MSG_Y2, names[i], c);
    }
}

void ui_clear_color_picker(void) {
    clear_area(0, MSG_Y1, 320, PICKER_CLR_H);
}

void ui_game_over_screen(unsigned char human_won, unsigned char winner_idx) {
    gfx_clear(GC_FELT);
    if (human_won) {
        text(124, 92, "YOU WIN!", GC_YELLOW);
        text(72, 114, "GREAT GAME, CHAMPION.", GC_WHITE);
    } else {
        text(120, 92, "GAME OVER", GC_RED);
        text(112, 114, "CPU", GC_WHITE);
        gfx_char(112 + 24, 114, (char)('0' + winner_idx), GC_WHITE);
        text(112 + 40, 114, "WINS", GC_WHITE);
    }
    text(68, 150, "PRESS SPACE TO PLAY AGAIN", GC_WHITE);
}

/* ---- event / prompt lines ---- */

void ui_draw_challenge_prompt(unsigned char victim, unsigned char player, unsigned char selected_yes) {
    int lx;
    clear_area(0, MSG_Y1, 320, 18);
    lx = player_label(4, MSG_Y1, player, GC_WHITE);
    text(lx, MSG_Y1, "PLAYED WILD DRAW FOUR", GC_WHITE);
    lx = player_label(4, MSG_Y2, victim, GC_WHITE);
    text(lx, MSG_Y2, "CHALLENGE?", GC_WHITE);
    text(124, MSG_Y2, "YES", selected_yes ? GC_YELLOW : GC_GRAY);
    text(164, MSG_Y2, "NO", selected_yes ? GC_GRAY : GC_YELLOW);
}

void ui_event_challenge_result(unsigned char victim, unsigned char player, unsigned char succeeded) {
    int lx;
    clear_area(0, MSG_Y1, 320, 18);
    lx = player_label(4, MSG_Y1, victim, GC_WHITE);
    text(lx, MSG_Y1, "CHALLENGES!", GC_WHITE);
    if (succeeded) {
        lx = player_label(4, MSG_Y2, player, GC_RED);
        text(lx, MSG_Y2, "HAD A MATCH - DRAWS 4", GC_RED);
    } else {
        lx = player_label(4, MSG_Y2, victim, GC_RED);
        text(lx, MSG_Y2, "WAS WRONG - DRAWS 6", GC_RED);
    }
}

void ui_event_skip(unsigned char idx) {
    int lx;
    clear_area(0, MSG_Y1, 320, 18);
    lx = player_label(4, MSG_Y1, idx, GC_WHITE);
    text(lx, MSG_Y1, idx == 0 ? "LOSE A TURN" : "IS SKIPPED", GC_WHITE);
}

void ui_event_reverse(unsigned char idx) {
    (void)idx;
    clear_area(0, MSG_Y1, 320, 18);
    text(4, MSG_Y1, "REVERSE! ORDER FLIPPED", GC_WHITE);
}

void ui_event_draw(unsigned char idx, unsigned char count) {
    const char *w = idx == 0 ? "MUST DRAW" : "DRAWS";
    int lx;
    clear_area(0, MSG_Y1, 320, 18);
    lx = player_label(4, MSG_Y1, idx, GC_WHITE);
    text(lx, MSG_Y1, w, GC_WHITE);
    put_num(lx + (int)strlen(w) * 8 + 8, MSG_Y1, count, GC_WHITE);
}

void ui_event_uno(unsigned char idx) {
    int lx;
    clear_area(0, MSG_Y2, 320, 8);
    lx = player_label(4, MSG_Y2, idx, GC_YELLOW);
    text(lx, MSG_Y2, "UNO! ONE CARD LEFT!", GC_YELLOW);
}

void ui_event_invalid(void) {
    clear_area(0, MSG_Y2, 320, 8);
    text(4, MSG_Y2, "THAT CARD DOES NOT MATCH!", GC_RED);
}

void ui_event_drew_one(unsigned char idx) {
    int lx;
    clear_area(0, MSG_Y1, 320, 18);
    lx = player_label(4, MSG_Y1, idx, GC_WHITE);
    text(lx, MSG_Y1, "DREW A CARD", GC_WHITE);
}

void ui_event_thinking(unsigned char idx) {
    int lx;
    clear_area(0, MSG_Y1, 320, 18);
    lx = player_label(4, MSG_Y1, idx, GC_WHITE);
    text(lx, MSG_Y1, "IS THINKING...", GC_WHITE);
}
