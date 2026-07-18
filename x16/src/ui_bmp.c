/* Bitmap UNO UI for the Commander X16 -- implements the shared ui.h
   interface (the same one the text-mode ui.c implements), but renders
   everything as pixel graphics through the vbmp layer (KERNAL GRAPH_* API)
   instead of text tiles. The game loop (main_bmp game) is UI-agnostic and
   calls these. */
#include "game.h"
#include "cards.h"
#include "ui.h"
#include "vbmp.h"

/* pixel layout for the 320x240 framebuffer */
#define TITLE_X 128
#define TITLE_Y 3
#define OPP_Y 18
#define TBL_LABEL_Y 34
#define CARD_Y 46
#define DRAW_X 26
#define TOP_X 108
#define INFO_X 176
#define MSG_Y1 124
#define MSG_Y2 136
#define HAND_TOP 148
#define HAND_Y 170

/* card colour (0-4) -> vbmp palette index */
static const unsigned char suit_gc[5] = {GC_RED, GC_YELLOW, GC_GREEN, GC_BLUE, GC_LTGRAY};
static const char *const color_name[4] = {"RED", "YELLOW", "GREEN", "BLUE"};

/* print an unsigned number at (x,y) in the current stroke colour */
static void put_num(unsigned int x, unsigned int y, unsigned int n) {
    char buf[6];
    unsigned char i = 0, j;
    if (n == 0) { gfx_char(x, y, '0'); return; }
    while (n && i < 5) { buf[i++] = (char)('0' + (n % 10)); n /= 10; }
    for (j = 0; j < i; j++) gfx_char(x + (unsigned int)j * 8, y, buf[i - 1 - j]);
}

/* fill a felt-coloured rectangle (erase a region) */
static void clear_area(unsigned int x, unsigned int y, unsigned int w, unsigned int h) {
    gfx_set_colors(GC_FELT, GC_FELT, GC_FELT);
    gfx_rect(x, y, w, h, 1);
}

static void text(unsigned int x, unsigned int y, const char *s, unsigned char color) {
    gfx_set_colors(color, color, GC_FELT);
    gfx_text(x, y, s);
}

void ui_title_screen(void) {
    gfx_init();
    gfx_set_colors(GC_FELT, GC_FELT, GC_FELT);
    gfx_rect(0, 0, 320, 240, 1);

    text(TITLE_X, 24, "U N O", GC_YELLOW);
    text(70, 44, "BITMAP EDITION - X16", GC_WHITE);
    text(52, 70, "1 PLAYER VS 3 CPU PLAYERS", GC_LTGRAY);
    text(60, 94, "L/R - PICK A CARD", GC_WHITE);
    text(60, 106, "FIRE/SPACE - PLAY", GC_WHITE);
    text(60, 118, "UP - DRAW A CARD", GC_WHITE);
    text(84, 142, "PRESS FIRE TO START", GC_YELLOW);
    /* sample cards along the bottom */
    vb_card(76, 166, 0, 5);
    vb_card(137, 166, 1, 7);
    vb_card(198, 166, 4, VAL_WILD);
}

void ui_draw_frame(void) {
    gfx_set_colors(GC_FELT, GC_FELT, GC_FELT);
    gfx_rect(0, 0, 320, 240, 1);
    text(TITLE_X, TITLE_Y, "U N O", GC_YELLOW);
}

void ui_draw_opponents(GameState *g) {
    unsigned char opp;
    clear_area(0, OPP_Y - 8, 320, 18);
    for (opp = 1; opp <= 3; opp++) {
        unsigned int x = 8 + (unsigned int)(opp - 1) * 106;
        unsigned char col = (g->current_player == opp) ? GC_YELLOW : GC_WHITE;
        text(x, OPP_Y, "CPU", col);
        gfx_set_colors(col, col, GC_FELT);
        gfx_char(x + 24, OPP_Y, (char)('0' + opp));
        gfx_char(x + 32, OPP_Y, ':');
        put_num(x + 42, OPP_Y, g->players[opp].count);
    }
}

void ui_draw_table(GameState *g) {
    clear_area(0, TBL_LABEL_Y - 8, 320, CARD_Y - TBL_LABEL_Y + 74);
    text(DRAW_X + 6, TBL_LABEL_Y, "DRAW", GC_LTGRAY);
    gfx_set_colors(GC_LTGRAY, GC_LTGRAY, GC_FELT);
    put_num(DRAW_X + 46, TBL_LABEL_Y, g->draw_count);
    vb_card_back(DRAW_X, CARD_Y);

    text(TOP_X - 4, TBL_LABEL_Y, "TOP CARD", GC_LTGRAY);
    vb_card(TOP_X, CARD_Y, g->top_card.color, g->top_card.value);

    text(INFO_X, CARD_Y + 8, "COLOR", GC_WHITE);
    text(INFO_X, CARD_Y + 20, color_name[g->top_color], suit_gc[g->top_color]);
    text(INFO_X, CARD_Y + 40, g->direction > 0 ? "DIR ->" : "DIR <-", GC_WHITE);
}

void ui_draw_hand(GameState *g, unsigned char cursor) {
    static unsigned char hs[HAND_VISIBLE], hv[HAND_VISIBLE];
    Player *p = &g->players[0];
    unsigned char i, n = p->count > HAND_VISIBLE ? HAND_VISIBLE : p->count;

    clear_area(0, HAND_TOP, 320, 240 - HAND_TOP);
    for (i = 0; i < n; i++) {
        hs[i] = p->hand[i].color;
        hv[i] = p->hand[i].value;
    }
    if (n) vb_hand(8, HAND_Y, hs, hv, n, cursor < n ? cursor : 0);
}

void ui_message(const char *line1, const char *line2) {
    clear_area(0, MSG_Y1 - 8, 320, 28);
    if (line1) text(8, MSG_Y1, line1, GC_WHITE);
    if (line2) text(8, MSG_Y2, line2, GC_WHITE);
}

void ui_draw_color_picker(unsigned char selected) {
    static const char *const names[4] = {"RED", "YELLOW", "GREEN", "BLUE"};
    unsigned char i;
    clear_area(0, MSG_Y1 - 8, 320, 28);
    text(8, MSG_Y1, "CHOOSE A COLOR:", GC_WHITE);
    for (i = 0; i < 4; i++) {
        unsigned int x = 8 + (unsigned int)i * 72;
        unsigned char c = suit_gc[i];
        if (i == selected) {
            gfx_set_colors(GC_YELLOW, GC_YELLOW, GC_FELT);
            gfx_rect(x - 2, MSG_Y2 - 2, 58, 12, 0);
        }
        gfx_set_colors(c, c, GC_FELT);
        gfx_rect(x, MSG_Y2, 12, 8, 1);
        text(x + 16, MSG_Y2, names[i], c);
    }
}

void ui_clear_color_picker(void) {
    clear_area(0, MSG_Y1 - 8, 320, 28);
}

void ui_game_over_screen(unsigned char human_won, unsigned char winner_idx) {
    gfx_set_colors(GC_FELT, GC_FELT, GC_FELT);
    gfx_rect(0, 0, 320, 240, 1);
    if (human_won) {
        text(120, 90, "YOU WIN!", GC_YELLOW);
        text(72, 110, "GREAT GAME, CHAMPION.", GC_WHITE);
    } else {
        text(116, 90, "GAME OVER", GC_RED);
        gfx_set_colors(GC_WHITE, GC_WHITE, GC_FELT);
        gfx_text(120, 110, "CPU");
        gfx_char(144, 110, (char)('0' + winner_idx));
        gfx_text(156, 110, " WINS");
    }
    text(76, 150, "PRESS FIRE TO PLAY AGAIN", GC_WHITE);
}

/* ---- event / prompt lines ---- */

static void player_label(unsigned int x, unsigned int y, unsigned char idx, unsigned char color) {
    if (idx == 0) { text(x, y, "YOU", color); return; }
    gfx_set_colors(color, color, GC_FELT);
    gfx_text(x, y, "CPU");
    gfx_char(x + 24, y, (char)('0' + idx));
}

void ui_draw_challenge_prompt(unsigned char victim, unsigned char player, unsigned char selected_yes) {
    clear_area(0, MSG_Y1 - 8, 320, 28);
    player_label(8, MSG_Y1, player, GC_WHITE);
    text(40, MSG_Y1, "PLAYED WILD DRAW FOUR", GC_WHITE);
    player_label(8, MSG_Y2, victim, GC_WHITE);
    text(40, MSG_Y2, "CHALLENGE?", GC_WHITE);
    text(120, MSG_Y2, "YES", selected_yes ? GC_YELLOW : GC_LTGRAY);
    text(160, MSG_Y2, "NO", selected_yes ? GC_LTGRAY : GC_YELLOW);
}

void ui_event_challenge_result(unsigned char victim, unsigned char player, unsigned char succeeded) {
    clear_area(0, MSG_Y1 - 8, 320, 28);
    player_label(8, MSG_Y1, victim, GC_WHITE);
    text(40, MSG_Y1, "CHALLENGES!", GC_WHITE);
    if (succeeded) {
        player_label(8, MSG_Y2, player, GC_RED);
        text(40, MSG_Y2, "HAD A MATCH - DRAWS 4", GC_RED);
    } else {
        player_label(8, MSG_Y2, victim, GC_RED);
        text(40, MSG_Y2, "WAS WRONG - DRAWS 6", GC_RED);
    }
}

void ui_event_skip(unsigned char idx) {
    clear_area(0, MSG_Y1 - 8, 320, 28);
    player_label(8, MSG_Y1, idx, GC_WHITE);
    text(40, MSG_Y1, idx == 0 ? "LOSE A TURN" : "IS SKIPPED", GC_WHITE);
}

void ui_event_reverse(unsigned char idx) {
    (void)idx;
    clear_area(0, MSG_Y1 - 8, 320, 28);
    text(8, MSG_Y1, "REVERSE! ORDER FLIPPED", GC_WHITE);
}

void ui_event_draw(unsigned char idx, unsigned char count) {
    clear_area(0, MSG_Y1 - 8, 320, 28);
    player_label(8, MSG_Y1, idx, GC_WHITE);
    text(40, MSG_Y1, idx == 0 ? "MUST DRAW" : "DRAWS", GC_WHITE);
    gfx_set_colors(GC_WHITE, GC_WHITE, GC_FELT);
    put_num(idx == 0 ? 120 : 88, MSG_Y1, count);
}

void ui_event_uno(unsigned char idx) {
    clear_area(0, MSG_Y2 - 8, 320, 16);
    player_label(8, MSG_Y2, idx, GC_YELLOW);
    text(40, MSG_Y2, "UNO! ONE CARD LEFT!", GC_YELLOW);
}

void ui_event_invalid(void) {
    clear_area(0, MSG_Y2 - 8, 320, 16);
    text(8, MSG_Y2, "THAT CARD DOES NOT MATCH!", GC_RED);
}

void ui_event_drew_one(unsigned char idx) {
    clear_area(0, MSG_Y1 - 8, 320, 28);
    player_label(8, MSG_Y1, idx, GC_WHITE);
    text(40, MSG_Y1, "DREW A CARD", GC_WHITE);
}

void ui_event_thinking(unsigned char idx) {
    clear_area(0, MSG_Y1 - 8, 320, 28);
    player_label(8, MSG_Y1, idx, GC_WHITE);
    text(40, MSG_Y1, "IS THINKING...", GC_WHITE);
}
