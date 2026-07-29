/* UNO UI for the Foenix F256K -- implements the shared ui.h using the Vicky
   per-cell colour text mode (f256vid.c). Cards are solid colour tiles with
   a contrasting value character, like the X16 (VERA) and Atari VBXE tile
   ports. 80x60 screen; the game uses the top ~24 rows and centres. */
#include <string.h>
#include "game.h"
#include "cards.h"
#include "ui.h"
#include "f256vid.h"

/* layout (columns 0-79, rows 0-59) */
#define TITLE_Y 1
#define OPP_Y 3
#define TBL_LABEL_Y 6
#define CARD_Y 7
#define DRAW_X 6
#define TOP_X 22
#define INFO_X 40
#define MSG_Y1 14
#define MSG_Y2 15
#define HAND_LABEL_Y 17
#define HAND_Y 19          /* hand tiles rows 19-21, caps at 18 and 22 */

#define BIG_W 8
#define BIG_H 5

#define FELT F256_COLOR(FC_WHITE, FC_FELT)

static const unsigned char suit_col[5] = {FC_RED, FC_YELLOW, FC_GREEN, FC_BLUE, FC_GRAY};
static const char *const color_name[4] = {"RED", "YELLOW", "GREEN", "BLUE"};

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

static void put_num(unsigned char x, unsigned char y, unsigned int n, unsigned char color) {
    char buf[6];
    unsigned char i = 0, j;
    if (n == 0) { f256_put(x, y, '0', color); return; }
    while (n && i < 5) { buf[i++] = (char)('0' + (n % 10)); n /= 10; }
    for (j = 0; j < i; j++) f256_put(x + j, y, (unsigned char)buf[i - 1 - j], color);
}

/* a colour-tile card: suit-coloured block, white value in the corners */
static void card_tile(unsigned char x, unsigned char y, unsigned char w, unsigned char h,
                      unsigned char suit, unsigned char value) {
    unsigned char col = F256_COLOR(FC_WHITE, suit_col[suit <= 4 ? suit : 4]);
    char vc = value_char(value);
    f256_fill(x, y, w, h, ' ', col);
    f256_put(x + 1, y, (unsigned char)vc, col);
    if (w >= 3 && h >= 2) f256_put(x + w - 2, y + h - 1, (unsigned char)vc, col);
    if (w >= 3 && h >= 3) f256_put(x + (w >> 1) - 1, y + (h >> 1), (unsigned char)vc, col);
}

static void back_tile(unsigned char x, unsigned char y, unsigned char w, unsigned char h) {
    f256_fill(x, y, w, h, '#', F256_COLOR(FC_WHITE, FC_BLUE));
}

static void clear_rows(unsigned char y, unsigned char h) {
    f256_fill(0, y, 80, h, ' ', FELT);
}

void ui_title_screen(void) {
    f256_clear(FELT);
    f256_puts(37, 6, "U N O", F256_COLOR(FC_YELLOW, FC_FELT));
    f256_puts(29, 8, "FOENIX  F256K  EDITION", F256_COLOR(FC_WHITE, FC_FELT));
    f256_puts(27, 11, "1 PLAYER VS 3 CPU PLAYERS", F256_COLOR(FC_GRAY, FC_FELT));
    f256_puts(30, 14, ", AND . - PICK A CARD", F256_COLOR(FC_WHITE, FC_FELT));
    f256_puts(30, 15, "SPACE - PLAY A CARD", F256_COLOR(FC_WHITE, FC_FELT));
    f256_puts(30, 16, "U - DRAW A CARD", F256_COLOR(FC_WHITE, FC_FELT));
    f256_puts(31, 19, "PRESS SPACE TO START", F256_COLOR(FC_YELLOW, FC_FELT));
    card_tile(30, 22, BIG_W, BIG_H, 0, 5);
    card_tile(40, 22, BIG_W, BIG_H, 4, VAL_WILD);
}

void ui_draw_frame(void) {
    f256_clear(FELT);
    f256_puts(37, TITLE_Y, "U N O", F256_COLOR(FC_YELLOW, FC_FELT));
}

void ui_draw_opponents(GameState *g) {
    unsigned char opp;
    clear_rows(OPP_Y, 1);
    for (opp = 1; opp <= 3; opp++) {
        unsigned char x = 8 + (opp - 1) * 24;
        unsigned char c = (g->current_player == opp) ? FC_YELLOW : FC_WHITE;
        unsigned char col = F256_COLOR(c, FC_FELT);
        f256_puts(x, OPP_Y, "CPU", col);
        f256_put(x + 3, OPP_Y, (unsigned char)('0' + opp), col);
        f256_put(x + 4, OPP_Y, ':', col);
        put_num(x + 6, OPP_Y, g->players[opp].count, col);
    }
}

void ui_draw_table(GameState *g) {
    unsigned char gray = F256_COLOR(FC_GRAY, FC_FELT);
    unsigned char white = F256_COLOR(FC_WHITE, FC_FELT);
    clear_rows(TBL_LABEL_Y, CARD_Y + BIG_H + 1 - TBL_LABEL_Y);

    f256_puts(DRAW_X, TBL_LABEL_Y, "DRAW", gray);
    put_num(DRAW_X + 5, TBL_LABEL_Y, g->draw_count, gray);
    back_tile(DRAW_X, CARD_Y, BIG_W, BIG_H);

    f256_puts(TOP_X, TBL_LABEL_Y, "TOP CARD", gray);
    card_tile(TOP_X, CARD_Y, BIG_W, BIG_H, g->top_card.color, g->top_card.value);

    f256_puts(INFO_X, CARD_Y, "COLOR", white);
    f256_puts(INFO_X, CARD_Y + 1, color_name[g->top_color],
              F256_COLOR(suit_col[g->top_color], FC_FELT));
    f256_puts(INFO_X, CARD_Y + 3, g->direction > 0 ? "DIR ->" : "DIR <-", white);
}

void ui_draw_hand(GameState *g, unsigned char cursor) {
    Player *p = &g->players[0];
    unsigned char n = p->count > HAND_VISIBLE ? HAND_VISIBLE : p->count;
    unsigned char i, stride, w, startx;

    f256_fill(0, HAND_Y - 1, 80, 5, ' ', FELT);
    if (n == 0) return;
    if (cursor >= n) cursor = 0;

    stride = (unsigned char)(76u / n);
    if (stride > 8) stride = 8;
    if (stride < 3) stride = 3;
    w = stride - 1;
    if (w < 2) w = 2;
    startx = (unsigned char)((80u - (unsigned int)n * stride) / 2);

    for (i = 0; i < n; i++) {
        unsigned char x = startx + i * stride;
        card_tile(x, HAND_Y, w, 3, p->hand[i].color, p->hand[i].value);
        if (i == cursor) {
            f256_fill(x, HAND_Y - 1, w, 1, ' ', F256_COLOR(FC_BLACK, FC_YELLOW));
            f256_fill(x, HAND_Y + 3, w, 1, ' ', F256_COLOR(FC_BLACK, FC_YELLOW));
        }
    }
}

void ui_message(const char *line1, const char *line2) {
    unsigned char w = F256_COLOR(FC_WHITE, FC_FELT);
    clear_rows(MSG_Y1, 2);
    if (line1) f256_puts(2, MSG_Y1, line1, w);
    if (line2) f256_puts(2, MSG_Y2, line2, w);
}

void ui_draw_color_picker(unsigned char selected) {
    static const char *const names[4] = {"RED", "YELLOW", "GREEN", "BLUE"};
    unsigned char i;
    clear_rows(MSG_Y1, 2);
    f256_puts(2, MSG_Y1, "CHOOSE A COLOR:", F256_COLOR(FC_WHITE, FC_FELT));
    for (i = 0; i < 4; i++) {
        unsigned char x = 2 + i * 16;
        unsigned char sc = suit_col[i];
        f256_fill(x, MSG_Y2, 2, 1, ' ', F256_COLOR(FC_WHITE, sc));
        f256_puts(x + 3, MSG_Y2, names[i],
                  F256_COLOR(i == selected ? FC_YELLOW : sc, FC_FELT));
    }
}

void ui_clear_color_picker(void) {
    clear_rows(MSG_Y1, 2);
}

void ui_game_over_screen(unsigned char human_won, unsigned char winner_idx) {
    f256_clear(FELT);
    if (human_won) {
        f256_puts(36, 10, "YOU WIN!", F256_COLOR(FC_YELLOW, FC_FELT));
        f256_puts(28, 12, "GREAT GAME, CHAMPION.", F256_COLOR(FC_WHITE, FC_FELT));
    } else {
        f256_puts(35, 10, "GAME OVER", F256_COLOR(FC_RED, FC_FELT));
        f256_puts(34, 12, "CPU", F256_COLOR(FC_WHITE, FC_FELT));
        f256_put(37, 12, (unsigned char)('0' + winner_idx), F256_COLOR(FC_WHITE, FC_FELT));
        f256_puts(39, 12, "WINS", F256_COLOR(FC_WHITE, FC_FELT));
    }
    f256_puts(28, 16, "PRESS SPACE TO PLAY AGAIN", F256_COLOR(FC_WHITE, FC_FELT));
}

/* ---- event / prompt lines ---- */

static unsigned char player_label(unsigned char x, unsigned char y, unsigned char idx, unsigned char c) {
    unsigned char col = F256_COLOR(c, FC_FELT);
    if (idx == 0) { f256_puts(x, y, "YOU", col); return x + 4; }
    f256_puts(x, y, "CPU", col);
    f256_put(x + 3, y, (unsigned char)('0' + idx), col);
    return x + 5;
}

void ui_draw_challenge_prompt(unsigned char victim, unsigned char player, unsigned char selected_yes) {
    unsigned char w = F256_COLOR(FC_WHITE, FC_FELT);
    unsigned char lx;
    clear_rows(MSG_Y1, 2);
    lx = player_label(2, MSG_Y1, player, FC_WHITE);
    f256_puts(lx, MSG_Y1, "PLAYED WILD DRAW FOUR", w);
    lx = player_label(2, MSG_Y2, victim, FC_WHITE);
    f256_puts(lx, MSG_Y2, "CHALLENGE?", w);
    f256_puts(30, MSG_Y2, "YES", F256_COLOR(selected_yes ? FC_YELLOW : FC_GRAY, FC_FELT));
    f256_puts(36, MSG_Y2, "NO", F256_COLOR(selected_yes ? FC_GRAY : FC_YELLOW, FC_FELT));
}

void ui_event_challenge_result(unsigned char victim, unsigned char player, unsigned char succeeded) {
    unsigned char r = F256_COLOR(FC_RED, FC_FELT);
    unsigned char lx;
    clear_rows(MSG_Y1, 2);
    lx = player_label(2, MSG_Y1, victim, FC_WHITE);
    f256_puts(lx, MSG_Y1, "CHALLENGES!", F256_COLOR(FC_WHITE, FC_FELT));
    if (succeeded) {
        lx = player_label(2, MSG_Y2, player, FC_RED);
        f256_puts(lx, MSG_Y2, "HAD A MATCH - DRAWS 4", r);
    } else {
        lx = player_label(2, MSG_Y2, victim, FC_RED);
        f256_puts(lx, MSG_Y2, "WAS WRONG - DRAWS 6", r);
    }
}

void ui_event_skip(unsigned char idx) {
    unsigned char lx;
    clear_rows(MSG_Y1, 1);
    lx = player_label(2, MSG_Y1, idx, FC_WHITE);
    f256_puts(lx, MSG_Y1, idx == 0 ? "LOSE A TURN" : "IS SKIPPED", F256_COLOR(FC_WHITE, FC_FELT));
}

void ui_event_reverse(unsigned char idx) {
    (void)idx;
    clear_rows(MSG_Y1, 1);
    f256_puts(2, MSG_Y1, "REVERSE! ORDER FLIPPED", F256_COLOR(FC_WHITE, FC_FELT));
}

void ui_event_draw(unsigned char idx, unsigned char count) {
    const char *ww = idx == 0 ? "MUST DRAW" : "DRAWS";
    unsigned char lx;
    clear_rows(MSG_Y1, 1);
    lx = player_label(2, MSG_Y1, idx, FC_WHITE);
    f256_puts(lx, MSG_Y1, ww, F256_COLOR(FC_WHITE, FC_FELT));
    put_num(lx + (unsigned char)strlen(ww) + 1, MSG_Y1, count, F256_COLOR(FC_WHITE, FC_FELT));
}

void ui_event_uno(unsigned char idx) {
    unsigned char lx;
    clear_rows(MSG_Y2, 1);
    lx = player_label(2, MSG_Y2, idx, FC_YELLOW);
    f256_puts(lx, MSG_Y2, "UNO! ONE CARD LEFT!", F256_COLOR(FC_YELLOW, FC_FELT));
}

void ui_event_invalid(void) {
    clear_rows(MSG_Y2, 1);
    f256_puts(2, MSG_Y2, "THAT CARD DOES NOT MATCH!", F256_COLOR(FC_RED, FC_FELT));
}

void ui_event_drew_one(unsigned char idx) {
    unsigned char lx;
    clear_rows(MSG_Y1, 1);
    lx = player_label(2, MSG_Y1, idx, FC_WHITE);
    f256_puts(lx, MSG_Y1, "DREW A CARD", F256_COLOR(FC_WHITE, FC_FELT));
}

void ui_event_thinking(unsigned char idx) {
    unsigned char lx;
    clear_rows(MSG_Y1, 1);
    lx = player_label(2, MSG_Y1, idx, FC_WHITE);
    f256_puts(lx, MSG_Y1, "IS THINKING...", F256_COLOR(FC_WHITE, FC_FELT));
}
