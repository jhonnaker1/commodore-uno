/* Bitmap UNO UI for the Atari VBXE -- implements the shared ui.h interface
   (the same one the text-mode ui_vbxe.c implements), but renders everything
   as pixel graphics through the vbxebmp layer (VBXE SR 320x192 8bpp
   framebuffer) instead of text tiles. The game loop (main_game_vbxebmp.c)
   is UI-agnostic and calls these. */
#include <string.h>
#include "game.h"
#include "cards.h"
#include "ui.h"
#include "vbxebmp.h"

/* pixel layout for the 320x192 framebuffer */
#define TITLE_X 132
#define TITLE_Y 2
#define OPP_Y 14
#define TBL_LABEL_Y 26
#define CARD_Y 36
#define DRAW_X 8
#define TOP_X 88
#define INFO_X 150
#define MSG_Y1 92
#define MSG_Y2 102
#define HAND_TOP 112
#define HAND_Y 128

/* card color (0-4) -> vbmp palette index */
static const unsigned char suit_vc[5] = {VC_RED, VC_YELLOW, VC_GREEN, VC_BLUE, VC_GRAY};
static const char *const color_name[4] = {"RED", "YELLOW", "GREEN", "BLUE"};

static void palette_init(void) {
    vbmp_palette(VC_BLACK, 0, 0, 0);
    vbmp_palette(VC_WHITE, 255, 255, 255);
    vbmp_palette(VC_RED, 200, 40, 40);
    vbmp_palette(VC_GREEN, 40, 170, 60);
    vbmp_palette(VC_BLUE, 70, 100, 220);
    vbmp_palette(VC_YELLOW, 230, 200, 40);
    vbmp_palette(VC_GRAY, 150, 150, 150);
    vbmp_palette(VC_SHADOW, 20, 40, 28);
    vbmp_palette(VC_FELT, 20, 90, 50);
}

/* print an unsigned number at (x,y) in colour col */
static void put_num(unsigned int x, unsigned char y, unsigned int n, unsigned char col) {
    char buf[6];
    unsigned char i = 0, j;
    if (n == 0) { vbmp_char(x, y, '0', col); return; }
    while (n && i < 5) { buf[i++] = (char)('0' + (n % 10)); n /= 10; }
    for (j = 0; j < i; j++) vbmp_char(x + (unsigned int)j * 8, y, (unsigned char)buf[i - 1 - j], col);
}

/* erase a felt-coloured region */
static void clear_area(unsigned int x, unsigned char y, unsigned int w, unsigned char h) {
    vbmp_fill_rect(x, y, w, h, VC_FELT);
}

static void text(unsigned int x, unsigned char y, const char *s, unsigned char col) {
    vbmp_text(x, y, s, col);
}

void ui_title_screen(void) {
    vbmp_init();
    palette_init();
    vbmp_clear(VC_FELT);

    text(TITLE_X, 16, "U N O", VC_YELLOW);
    text(56, 34, "BITMAP EDITION - VBXE", VC_WHITE);
    text(48, 56, "1 PLAYER VS 3 CPU PLAYERS", VC_GRAY);
    text(64, 78, "L/R - PICK A CARD", VC_WHITE);
    text(64, 90, "FIRE/SPACE - PLAY", VC_WHITE);
    text(64, 102, "UP - DRAW A CARD", VC_WHITE);
    text(84, 124, "PRESS FIRE TO START", VC_YELLOW);
    /* sample cards along the bottom */
    vbmp_card(72, 144, 0, 5);
    vbmp_card(128, 144, 1, 7);
    vbmp_card(184, 144, 4, VAL_WILD);
}

void ui_draw_frame(void) {
    vbmp_clear(VC_FELT);
    text(TITLE_X, TITLE_Y, "U N O", VC_YELLOW);
}

void ui_draw_opponents(GameState *g) {
    unsigned char opp;
    clear_area(0, OPP_Y, 320, 8);
    for (opp = 1; opp <= 3; opp++) {
        unsigned int x = 8 + (unsigned int)(opp - 1) * 106;
        unsigned char col = (g->current_player == opp) ? VC_YELLOW : VC_WHITE;
        text(x, OPP_Y, "CPU", col);
        vbmp_char(x + 24, OPP_Y, (unsigned char)('0' + opp), col);
        vbmp_char(x + 32, OPP_Y, ':', col);
        put_num(x + 42, OPP_Y, g->players[opp].count, col);
    }
}

void ui_draw_table(GameState *g) {
    clear_area(0, TBL_LABEL_Y, 320, (unsigned char)(CARD_Y + VBMP_CARD_H + 6 - TBL_LABEL_Y));
    text(DRAW_X + 4, TBL_LABEL_Y, "DRAW", VC_GRAY);
    put_num(DRAW_X + 40, TBL_LABEL_Y, g->draw_count, VC_GRAY);
    vbmp_card_back(DRAW_X, CARD_Y);

    text(TOP_X, TBL_LABEL_Y, "TOP CARD", VC_GRAY);
    vbmp_card(TOP_X, CARD_Y, g->top_card.color, g->top_card.value);

    text(INFO_X, CARD_Y + 4, "COLOR", VC_WHITE);
    text(INFO_X, CARD_Y + 16, color_name[g->top_color], suit_vc[g->top_color]);
    text(INFO_X, CARD_Y + 34, g->direction > 0 ? "DIR ->" : "DIR <-", VC_WHITE);
}

void ui_draw_hand(GameState *g, unsigned char cursor) {
    static unsigned char hs[HAND_VISIBLE], hv[HAND_VISIBLE];
    Player *p = &g->players[0];
    unsigned char i, n = p->count > HAND_VISIBLE ? HAND_VISIBLE : p->count;

    clear_area(0, HAND_TOP, 320, (unsigned char)(BMP_H - HAND_TOP));
    for (i = 0; i < n; i++) {
        hs[i] = p->hand[i].color;
        hv[i] = p->hand[i].value;
    }
    if (n) vbmp_hand(6, HAND_Y, hs, hv, n, cursor < n ? cursor : 0);
}

void ui_message(const char *line1, const char *line2) {
    clear_area(0, MSG_Y1, 320, 18);
    if (line1) text(4, MSG_Y1, line1, VC_WHITE);
    if (line2) text(4, MSG_Y2, line2, VC_WHITE);
}

/* The selection frame is 12px tall at MSG_Y2-2 (=100), so its bottom edge
   reaches y=111 -- below the 18px-tall message band. Clear 20px (through
   y=111, just above the hand at HAND_TOP=112) so a moved/cleared selection
   leaves no bracket-bottom behind. */
#define PICKER_CLR_H 20

void ui_draw_color_picker(unsigned char selected) {
    static const char *const names[4] = {"RED", "YELLOW", "GREEN", "BLUE"};
    unsigned char i;
    clear_area(0, MSG_Y1, 320, PICKER_CLR_H);
    text(4, MSG_Y1, "CHOOSE A COLOR:", VC_WHITE);
    for (i = 0; i < 4; i++) {
        unsigned int x = 4 + (unsigned int)i * 76;
        unsigned char c = suit_vc[i];
        /* Frame must clear the widest label ("YELLOW", 6 chars): swatch(12)
           + gap(4) + 48 = ends at x+64, so span x-2..x+68 (width 70). */
        if (i == selected) vbmp_frame_rect(x - 2, MSG_Y2 - 2, 70, 12, VC_YELLOW);
        vbmp_fill_rect(x, MSG_Y2, 12, 8, c);
        text(x + 16, MSG_Y2, names[i], c);
    }
}

void ui_clear_color_picker(void) {
    clear_area(0, MSG_Y1, 320, PICKER_CLR_H);
}

void ui_game_over_screen(unsigned char human_won, unsigned char winner_idx) {
    vbmp_clear(VC_FELT);
    if (human_won) {
        text(124, 70, "YOU WIN!", VC_YELLOW);
        text(72, 92, "GREAT GAME, CHAMPION.", VC_WHITE);
    } else {
        text(120, 70, "GAME OVER", VC_RED);
        text(112, 92, "CPU", VC_WHITE);
        vbmp_char(112 + 24, 92, (unsigned char)('0' + winner_idx), VC_WHITE);
        text(112 + 40, 92, "WINS", VC_WHITE);
    }
    text(72, 128, "PRESS FIRE TO PLAY AGAIN", VC_WHITE);
}

/* ---- event / prompt lines ---- */

/* Draw the player's label ("YOU" or "CPUn") and return the x for the text
   that follows it, leaving a one-character gap so e.g. "CPU1" and the message
   never run together. */
static unsigned int player_label(unsigned int x, unsigned char y, unsigned char idx, unsigned char color) {
    if (idx == 0) { text(x, y, "YOU", color); return x + 4 * 8; }  /* "YOU" + space */
    text(x, y, "CPU", color);
    vbmp_char(x + 24, y, (unsigned char)('0' + idx), color);
    return x + 5 * 8;                                              /* "CPUn" + space */
}

void ui_draw_challenge_prompt(unsigned char victim, unsigned char player, unsigned char selected_yes) {
    unsigned int lx;
    clear_area(0, MSG_Y1, 320, 18);
    lx = player_label(4, MSG_Y1, player, VC_WHITE);
    text(lx, MSG_Y1, "PLAYED WILD DRAW FOUR", VC_WHITE);
    lx = player_label(4, MSG_Y2, victim, VC_WHITE);
    text(lx, MSG_Y2, "CHALLENGE?", VC_WHITE);
    text(124, MSG_Y2, "YES", selected_yes ? VC_YELLOW : VC_GRAY);
    text(164, MSG_Y2, "NO", selected_yes ? VC_GRAY : VC_YELLOW);
}

void ui_event_challenge_result(unsigned char victim, unsigned char player, unsigned char succeeded) {
    unsigned int lx;
    clear_area(0, MSG_Y1, 320, 18);
    lx = player_label(4, MSG_Y1, victim, VC_WHITE);
    text(lx, MSG_Y1, "CHALLENGES!", VC_WHITE);
    if (succeeded) {
        lx = player_label(4, MSG_Y2, player, VC_RED);
        text(lx, MSG_Y2, "HAD A MATCH - DRAWS 4", VC_RED);
    } else {
        lx = player_label(4, MSG_Y2, victim, VC_RED);
        text(lx, MSG_Y2, "WAS WRONG - DRAWS 6", VC_RED);
    }
}

void ui_event_skip(unsigned char idx) {
    unsigned int lx;
    clear_area(0, MSG_Y1, 320, 18);
    lx = player_label(4, MSG_Y1, idx, VC_WHITE);
    text(lx, MSG_Y1, idx == 0 ? "LOSE A TURN" : "IS SKIPPED", VC_WHITE);
}

void ui_event_reverse(unsigned char idx) {
    (void)idx;
    clear_area(0, MSG_Y1, 320, 18);
    text(4, MSG_Y1, "REVERSE! ORDER FLIPPED", VC_WHITE);
}

void ui_event_draw(unsigned char idx, unsigned char count) {
    const char *w = idx == 0 ? "MUST DRAW" : "DRAWS";
    unsigned int lx;
    clear_area(0, MSG_Y1, 320, 18);
    lx = player_label(4, MSG_Y1, idx, VC_WHITE);
    text(lx, MSG_Y1, w, VC_WHITE);
    put_num(lx + (unsigned int)strlen(w) * 8 + 8, MSG_Y1, count, VC_WHITE);
}

void ui_event_uno(unsigned char idx) {
    unsigned int lx;
    clear_area(0, MSG_Y2, 320, 8);
    lx = player_label(4, MSG_Y2, idx, VC_YELLOW);
    text(lx, MSG_Y2, "UNO! ONE CARD LEFT!", VC_YELLOW);
}

void ui_event_invalid(void) {
    clear_area(0, MSG_Y2, 320, 8);
    text(4, MSG_Y2, "THAT CARD DOES NOT MATCH!", VC_RED);
}

void ui_event_drew_one(unsigned char idx) {
    unsigned int lx;
    clear_area(0, MSG_Y1, 320, 18);
    lx = player_label(4, MSG_Y1, idx, VC_WHITE);
    text(lx, MSG_Y1, "DREW A CARD", VC_WHITE);
}

void ui_event_thinking(unsigned char idx) {
    unsigned int lx;
    clear_area(0, MSG_Y1, 320, 18);
    lx = player_label(4, MSG_Y1, idx, VC_WHITE);
    text(lx, MSG_Y1, "IS THINKING...", VC_WHITE);
}
