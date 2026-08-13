/* UNO's screen for the MSX2: pixel-art cards on a 256x212 SCREEN 5 bitmap,
   drawn through the V9938 layer in msxvdp.c. Same card design as the Atari
   ST, Amiga and X16/VBXE bitmap builds, re-laid-out for the narrower
   screen and with the suit colours set to the real UNO colours rather than
   the nearest available -- which on this machine is a palette entry away. */
#include <string.h>
#include "game.h"
#include "cards.h"
#include "ui.h"
#include "msxvdp.h"

#define TITLE_X 108
#define TITLE_Y 4
#define OPP_Y 16
#define TBL_LABEL_Y 30
#define CARD_Y 42
#define DRAW_X 8
#define TOP_X 72
#define INFO_X 128
#define MSG_Y1 92
#define MSG_Y2 104
#define HAND_TOP 118
#define HAND_Y 150

/* The colour picker's selection frame sits at MSG_Y2-2 and stands 12 pixels
   tall, so the message area has to be cleared down to row 116 or its bottom
   edge is left behind as a stray line. HAND_TOP is 118, which leaves room. */
#define PICKER_CLR_H 24

static const unsigned char suit_gc[5] = {
    GC_RED, GC_YELLOW, GC_GREEN, GC_BLUE, GC_GREY
};
static const char *const color_name[4] = {"RED", "YELLOW", "GREEN", "BLUE"};

static void text(int x, int y, const char *s, unsigned char col) {
    gfx_text(x, y, s, col, GC_FELT);
}

static void put_num(int x, int y, unsigned int n, unsigned char col) {
    char buf[6];
    unsigned char i = 0, j;
    if (n == 0) { gfx_char(x, y, '0', col, GC_FELT); return; }
    while (n && i < 5) { buf[i++] = (char)('0' + (n % 10)); n /= 10; }
    for (j = 0; j < i; j++)
        gfx_char(x + (int)j * 8, y, buf[i - 1 - j], col, GC_FELT);
}

static void clear_area(int x, int y, int w, int h) {
    gfx_fill_rect(x, y, w, h, GC_FELT);
}

/* Draw a player's label ("YOU" or "CPUn") and return the x the following
   text should start at, leaving a one-character gap. */
static int player_label(int x, int y, unsigned char idx, unsigned char color) {
    if (idx == 0) { text(x, y, "YOU", color); return x + 4 * 8; }
    text(x, y, "CPU", color);
    gfx_char(x + 24, y, (char)('0' + idx), color, GC_FELT);
    return x + 5 * 8;
}

void ui_title_screen(void) {
    gfx_clear(GC_FELT);
    text(TITLE_X, 20, "U N O", GC_YELLOW);
    text(80, 36, "MSX2 EDITION", GC_WHITE);
    text(28, 56, "1 PLAYER VS 3 CPU PLAYERS", GC_GREY);
    text(48, 76, "ARROWS - PICK A CARD", GC_WHITE);
    text(52, 88, "SPACE  - PLAY A CARD", GC_WHITE);
    text(48, 100, "U / UP - DRAW A CARD", GC_WHITE);
    text(48, 120, "PRESS SPACE TO START", GC_YELLOW);
    gfx_card(60, 144, 0, 5);
    gfx_card(114, 144, 1, 7);
    gfx_card(168, 144, 4, VAL_WILD);
}

void ui_draw_frame(void) {
    gfx_clear(GC_FELT);
    text(TITLE_X, TITLE_Y, "U N O", GC_YELLOW);
}

void ui_draw_opponents(GameState *g) {
    unsigned char opp;
    clear_area(0, OPP_Y, GFX_W, 8);
    for (opp = 1; opp <= 3; opp++) {
        int x = 4 + (int)(opp - 1) * 84;
        unsigned char col = (g->current_player == opp) ? GC_YELLOW : GC_WHITE;
        text(x, OPP_Y, "CPU", col);
        gfx_char(x + 24, OPP_Y, (char)('0' + opp), col, GC_FELT);
        gfx_char(x + 32, OPP_Y, ':', col, GC_FELT);
        put_num(x + 42, OPP_Y, g->players[opp].count, col);
    }
}

void ui_draw_table(GameState *g) {
    clear_area(0, TBL_LABEL_Y, GFX_W,
               CARD_Y + GFX_CARD_H + 6 - TBL_LABEL_Y);
    text(DRAW_X, TBL_LABEL_Y, "DRAW", GC_GREY);
    put_num(DRAW_X + 36, TBL_LABEL_Y, g->draw_count, GC_GREY);
    gfx_card_back(DRAW_X, CARD_Y);

    text(TOP_X, TBL_LABEL_Y, "TOP CARD", GC_GREY);
    gfx_card(TOP_X, CARD_Y, g->top_card.color, g->top_card.value);

    text(INFO_X, CARD_Y + 4, "COLOR", GC_WHITE);
    text(INFO_X, CARD_Y + 16, color_name[g->top_color], suit_gc[g->top_color]);
    text(INFO_X, CARD_Y + 32, g->direction > 0 ? "DIR ->" : "DIR <-", GC_WHITE);
}

void ui_draw_hand(GameState *g, unsigned char cursor) {
    static unsigned char hs[HAND_VISIBLE], hv[HAND_VISIBLE];
    Player *p = &g->players[0];
    unsigned char i, n = p->count > HAND_VISIBLE ? HAND_VISIBLE : p->count;

    clear_area(0, HAND_TOP, GFX_W, GFX_H - HAND_TOP);
    for (i = 0; i < n; i++) {
        hs[i] = p->hand[i].color;
        hv[i] = p->hand[i].value;
    }
    if (n) gfx_hand(6, HAND_Y, hs, hv, n, cursor < n ? cursor : 0);
}

void ui_message(const char *line1, const char *line2) {
    clear_area(0, MSG_Y1, GFX_W, 20);
    if (line1) text(4, MSG_Y1, line1, GC_WHITE);
    if (line2) text(4, MSG_Y2, line2, GC_WHITE);
}

void ui_draw_color_picker(unsigned char selected) {
    unsigned char i;
    clear_area(0, MSG_Y1, GFX_W, PICKER_CLR_H);
    text(4, MSG_Y1, "CHOOSE A COLOR:", GC_WHITE);
    for (i = 0; i < 4; i++) {
        int x = 2 + (int)i * 63;
        unsigned char c = suit_gc[i];
        if (i == selected) gfx_frame_rect(x - 2, MSG_Y2 - 2, 62, 12, GC_YELLOW);
        gfx_fill_rect(x, MSG_Y2, 10, 8, c);
        text(x + 12, MSG_Y2, color_name[i], c);
    }
}

void ui_clear_color_picker(void) {
    clear_area(0, MSG_Y1, GFX_W, PICKER_CLR_H);
}

void ui_game_over_screen(unsigned char human_won, unsigned char winner_idx) {
    gfx_clear(GC_FELT);
    if (human_won) {
        text(96, 80, "YOU WIN!", GC_YELLOW);
        text(44, 100, "GREAT GAME, CHAMPION.", GC_WHITE);
    } else {
        text(92, 80, "GAME OVER", GC_RED);
        text(92, 100, "CPU", GC_WHITE);
        gfx_char(92 + 24, 100, (char)('0' + winner_idx), GC_WHITE, GC_FELT);
        text(92 + 40, 100, "WINS", GC_WHITE);
    }
    text(28, 140, "PRESS SPACE TO PLAY AGAIN", GC_WHITE);
}

/* ---- event / prompt lines ---- */

void ui_draw_challenge_prompt(unsigned char victim, unsigned char player,
                              unsigned char selected_yes) {
    int lx;
    clear_area(0, MSG_Y1, GFX_W, 20);
    lx = player_label(4, MSG_Y1, player, GC_WHITE);
    text(lx, MSG_Y1, "PLAYED WILD DRAW FOUR", GC_WHITE);
    /* No "YOU" label on this line: the question is plainly addressed to the
       player, and dropping it frees the 32 pixels the key hint needs. That
       hint matters here more than anywhere else in the game -- this is the
       one modal prompt with no other way to find out what to press. */
    (void)victim;
    text(4, MSG_Y2, "CHALLENGE?", GC_WHITE);
    text(92, MSG_Y2, "YES", selected_yes ? GC_YELLOW : GC_GREY);
    text(132, MSG_Y2, "NO", selected_yes ? GC_GREY : GC_YELLOW);
    text(164, MSG_Y2, "SPACE/RET", GC_GREY);
}

void ui_event_challenge_result(unsigned char victim, unsigned char player,
                               unsigned char succeeded) {
    int lx;
    clear_area(0, MSG_Y1, GFX_W, 20);
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
    clear_area(0, MSG_Y1, GFX_W, 20);
    lx = player_label(4, MSG_Y1, idx, GC_WHITE);
    text(lx, MSG_Y1, idx == 0 ? "LOSE A TURN" : "IS SKIPPED", GC_WHITE);
}

void ui_event_reverse(unsigned char idx) {
    (void)idx;
    clear_area(0, MSG_Y1, GFX_W, 20);
    text(4, MSG_Y1, "REVERSE! ORDER FLIPPED", GC_WHITE);
}

void ui_event_draw(unsigned char idx, unsigned char count) {
    const char *w = idx == 0 ? "MUST DRAW" : "DRAWS";
    int lx;
    clear_area(0, MSG_Y1, GFX_W, 20);
    lx = player_label(4, MSG_Y1, idx, GC_WHITE);
    text(lx, MSG_Y1, w, GC_WHITE);
    put_num(lx + (int)strlen(w) * 8 + 8, MSG_Y1, count, GC_WHITE);
}

void ui_event_uno(unsigned char idx) {
    int lx;
    clear_area(0, MSG_Y2, GFX_W, 8);
    lx = player_label(4, MSG_Y2, idx, GC_YELLOW);
    text(lx, MSG_Y2, "UNO! ONE CARD LEFT!", GC_YELLOW);
}

void ui_event_invalid(void) {
    clear_area(0, MSG_Y2, GFX_W, 8);
    text(4, MSG_Y2, "THAT CARD DOES NOT MATCH!", GC_RED);
}

void ui_event_drew_one(unsigned char idx) {
    int lx;
    clear_area(0, MSG_Y1, GFX_W, 20);
    lx = player_label(4, MSG_Y1, idx, GC_WHITE);
    text(lx, MSG_Y1, "DREW A CARD", GC_WHITE);
}

void ui_event_thinking(unsigned char idx) {
    int lx;
    clear_area(0, MSG_Y1, GFX_W, 20);
    lx = player_label(4, MSG_Y1, idx, GC_WHITE);
    text(lx, MSG_Y1, "IS THINKING...", GC_WHITE);
}
