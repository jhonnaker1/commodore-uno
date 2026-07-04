#include <ascii_charmap.h>
#include "ui.h"
#include "ted.h"

#define TITLE_Y 0
#define OPP_Y 2
#define TABLE_Y 4
#define TABLE_INFO_Y 5
#define MSG_Y1 7
#define MSG_Y2 8
#define HAND_LABEL_Y 10
#define HAND_Y 11

static const unsigned char suit_color[4] = {COL_RED, COL_YELLOW, COL_GREEN, COL_BLUE};

/* '1'-'9', '0', then 'A'-'J' for slots 0-19 (matches the quick-play keys). */
static char label_char(unsigned char idx) {
    if (idx < 9) return (char)('1' + idx);
    if (idx == 9) return '0';
    return (char)('A' + (idx - 10));
}

/* Single-character value glyph for a card, using the stock font. */
static char value_char(Card c) {
    if (c.value <= 9) return (char)('0' + c.value);
    if (c.value == VAL_SKIP) return 'S';
    if (c.value == VAL_REVERSE) return 'V';
    if (c.value == VAL_DRAW2) return 'D';
    if (c.value == VAL_WILD) return 'W';
    return 'F';
}

static unsigned char card_color(Card c, unsigned char color_override) {
    if (c.color != COLOR_WILD) return suit_color[c.color];
    if (color_override != NONE) return suit_color[color_override];
    return COL_LTGRAY;
}

void ui_title_screen(void) {
    scr_clear();
    scr_puts(15, 3, "U N O", COL_YELLOW);
    scr_puts(4, 5, "FOR THE COMMODORE PLUS/4", COL_WHITE);
    scr_puts(6, 10, "1 PLAYER VS 3 COMPUTER PLAYERS", COL_LTGRAY);
    scr_puts(9, 12, "JOYSTICK IN PORT 1, OR:", COL_CYAN);
    scr_puts(6, 13, "CRSR LEFT/RIGHT: PICK A CARD", COL_CYAN);
    scr_puts(6, 14, "SPACE OR RETURN: PLAY / CONFIRM", COL_CYAN);
    scr_puts(6, 15, "CRSR UP: DRAW A CARD", COL_CYAN);
    scr_puts(2, 16, "OR PRESS 1-9,0,A-J: PLAY THAT CARD", COL_CYAN);
    scr_puts(2, 18, "CARDS SHOW AS COLOR + LETTER/DIGIT:", COL_LTGRAY);
    scr_puts(2, 19, "S=SKIP  V=REVERSE  D=DRAW2", COL_LTGRAY);
    scr_puts(2, 20, "W=WILD  F=WILD+4", COL_LTGRAY);
    scr_puts(9, 22, "PRESS FIRE TO START", COL_GREEN);
}

void ui_draw_frame(void) {
    scr_clear();
    scr_puts(14, TITLE_Y, "*** U N O ***", COL_YELLOW);
}

void ui_draw_opponents(GameState *g) {
    unsigned char opp, x;
    scr_fill_rect(0, OPP_Y, COLS, 1, 32, COL_BLACK);
    for (opp = 1; opp <= 3; opp++) {
        x = 1 + (opp - 1) * 13;
        scr_put(x, OPP_Y, 'C', COL_WHITE);
        scr_put(x + 1, OPP_Y, 'P', COL_WHITE);
        scr_put(x + 2, OPP_Y, 'U', COL_WHITE);
        scr_put(x + 3, OPP_Y, '0' + opp, COL_WHITE);
        scr_put(x + 4, OPP_Y, ':', COL_WHITE);
        scr_put_num(x + 5, OPP_Y, g->players[opp].count, COL_LTGRAY);
        scr_put(x + 8, OPP_Y, g->current_player == opp ? '<' : ' ', COL_YELLOW);
    }
}

void ui_draw_table(GameState *g) {
    scr_fill_rect(0, TABLE_Y, COLS, 2, 32, COL_BLACK);
    scr_puts(1, TABLE_Y, "DRAW:", COL_LTGRAY);
    scr_puts(6, TABLE_Y, "x", COL_LTGRAY);
    scr_put_num(7, TABLE_Y, g->draw_count, COL_LTGRAY);

    scr_puts(13, TABLE_Y, "TOP:", COL_LTGRAY);
    scr_put(18, TABLE_Y, '[', COL_WHITE);
    scr_put(19, TABLE_Y, value_char(g->top_card), card_color(g->top_card, g->top_color));
    scr_put(20, TABLE_Y, ']', COL_WHITE);

    scr_puts(23, TABLE_Y, "COL:", COL_WHITE);
    scr_put(28, TABLE_Y, 160, suit_color[g->top_color]); /* solid-ish block from stock font */

    scr_puts(30, TABLE_Y, g->direction > 0 ? "DIR ->" : "DIR <-", COL_WHITE);
}

void ui_draw_hand(GameState *g, unsigned char cursor) {
    Player *p = &g->players[0];
    unsigned char i, x, y, shown;

    scr_fill_rect(0, HAND_LABEL_Y, COLS, 1, 32, COL_BLACK);
    scr_puts(1, HAND_LABEL_Y, "YOUR HAND", COL_WHITE);
    scr_put_num(11, HAND_LABEL_Y, p->count, COL_WHITE);

    scr_fill_rect(0, HAND_Y, COLS, 3, 32, COL_BLACK);

    shown = (p->count > HAND_VISIBLE) ? HAND_VISIBLE : p->count;

    for (i = 0; i < shown; i++) {
        x = 1 + (i % 7) * 5;
        y = (unsigned char)(HAND_Y + i / 7);
        if (i == cursor) {
            scr_put(x, y, '[', COL_YELLOW);
            scr_put(x + 1, y, label_char(i), COL_YELLOW);
            scr_put(x + 2, y, ':', COL_YELLOW);
            scr_put(x + 3, y, value_char(p->hand[i]), card_color(p->hand[i], NONE));
            scr_put(x + 4, y, ']', COL_YELLOW);
        } else {
            scr_put(x, y, ' ', COL_BLACK);
            scr_put(x + 1, y, label_char(i), COL_LTGRAY);
            scr_put(x + 2, y, ':', COL_LTGRAY);
            scr_put(x + 3, y, value_char(p->hand[i]), card_color(p->hand[i], NONE));
            scr_put(x + 4, y, ' ', COL_BLACK);
        }
    }
}

void ui_message(const char *line1, const char *line2) {
    scr_fill_rect(0, MSG_Y1, COLS, 2, 32, COL_BLACK);
    if (line1) scr_puts(1, MSG_Y1, line1, COL_WHITE);
    if (line2) scr_puts(1, MSG_Y2, line2, COL_WHITE);
}

void ui_draw_color_picker(unsigned char selected) {
    static const char *names[4] = {"RED", "YELLOW", "GREEN", "BLUE"};
    unsigned char i, x;

    scr_fill_rect(0, MSG_Y1, COLS, 2, 32, COL_BLACK);
    scr_puts(1, MSG_Y1, "CHOOSE A COLOR:", COL_WHITE);
    for (i = 0; i < 4; i++) {
        x = 2 + i * 9;
        scr_put(x, MSG_Y2, selected == i ? '[' : 32, COL_WHITE);
        scr_puts(x + 1, MSG_Y2, names[i], suit_color[i]);
        scr_put(x + 1 + 6, MSG_Y2, selected == i ? ']' : 32, COL_WHITE);
    }
}

void ui_clear_color_picker(void) {
    scr_fill_rect(0, MSG_Y1, COLS, 2, 32, COL_BLACK);
}

static unsigned char player_label(unsigned char x, unsigned char row, unsigned char idx) {
    if (idx == 0) {
        scr_puts(x, row, "YOU", COL_YELLOW);
        return x + 4;
    }
    scr_puts(x, row, "CPU", COL_YELLOW);
    scr_put(x + 3, row, '0' + idx, COL_YELLOW);
    return x + 5;
}

void ui_event_skip(unsigned char idx) {
    unsigned char x;
    scr_fill_rect(0, MSG_Y1, COLS, 2, 32, COL_BLACK);
    x = player_label(1, MSG_Y1, idx);
    scr_puts(x, MSG_Y1, idx == 0 ? "LOSE A TURN (SKIPPED)" : "IS SKIPPED", COL_WHITE);
}

void ui_event_reverse(unsigned char idx) {
    (void)idx;
    scr_fill_rect(0, MSG_Y1, COLS, 2, 32, COL_BLACK);
    scr_puts(1, MSG_Y1, "REVERSE! PLAY ORDER FLIPPED", COL_WHITE);
}

void ui_event_draw(unsigned char idx, unsigned char count) {
    unsigned char x;
    scr_fill_rect(0, MSG_Y1, COLS, 2, 32, COL_BLACK);
    x = player_label(1, MSG_Y1, idx);
    scr_puts(x, MSG_Y1, idx == 0 ? "MUST DRAW" : "DRAWS", COL_WHITE);
    scr_put_num(x + (idx == 0 ? 10 : 6), MSG_Y1, count, COL_WHITE);
}

void ui_event_uno(unsigned char idx) {
    unsigned char x;
    scr_fill_rect(0, MSG_Y2, COLS, 1, 32, COL_BLACK);
    x = player_label(1, MSG_Y2, idx);
    scr_puts(x, MSG_Y2, "UNO! ONE CARD LEFT!", COL_YELLOW);
}

void ui_event_invalid(void) {
    scr_fill_rect(0, MSG_Y2, COLS, 1, 32, COL_BLACK);
    scr_puts(1, MSG_Y2, "THAT CARD DOES NOT MATCH!", COL_RED);
}

void ui_event_drew_one(unsigned char idx) {
    unsigned char x;
    scr_fill_rect(0, MSG_Y1, COLS, 2, 32, COL_BLACK);
    x = player_label(1, MSG_Y1, idx);
    scr_puts(x, MSG_Y1, "NO LEGAL CARD, DREW ONE", COL_WHITE);
}

void ui_event_thinking(unsigned char idx) {
    unsigned char x;
    scr_fill_rect(0, MSG_Y1, COLS, 2, 32, COL_BLACK);
    x = player_label(1, MSG_Y1, idx);
    scr_puts(x, MSG_Y1, "IS THINKING...", COL_LTGRAY);
}

void ui_draw_challenge_prompt(unsigned char victim, unsigned char player, unsigned char selected_yes) {
    unsigned char x;
    scr_fill_rect(0, MSG_Y1, COLS, 2, 32, COL_BLACK);
    x = player_label(1, MSG_Y1, player);
    scr_puts(x, MSG_Y1, "PLAYED WILD DRAW FOUR", COL_WHITE);
    x = player_label(1, MSG_Y2, victim);
    scr_puts(x, MSG_Y2, "CHALLENGE?", COL_WHITE);
    x += 11;
    scr_put(x, MSG_Y2, selected_yes ? '[' : 32, COL_WHITE);
    scr_puts(x + 1, MSG_Y2, "YES", selected_yes ? COL_GREEN : COL_LTGRAY);
    scr_put(x + 4, MSG_Y2, selected_yes ? ']' : 32, COL_WHITE);
    scr_put(x + 6, MSG_Y2, !selected_yes ? '[' : 32, COL_WHITE);
    scr_puts(x + 7, MSG_Y2, "NO", !selected_yes ? COL_RED : COL_LTGRAY);
    scr_put(x + 9, MSG_Y2, !selected_yes ? ']' : 32, COL_WHITE);
}

void ui_event_challenge_result(unsigned char victim, unsigned char player, unsigned char succeeded) {
    unsigned char x;
    scr_fill_rect(0, MSG_Y1, COLS, 2, 32, COL_BLACK);
    x = player_label(1, MSG_Y1, victim);
    scr_puts(x, MSG_Y1, "CHALLENGES!", COL_YELLOW);
    if (succeeded) {
        x = player_label(1, MSG_Y2, player);
        scr_puts(x, MSG_Y2, "HAD A MATCH - DRAWS 4", COL_GREEN);
    } else {
        x = player_label(1, MSG_Y2, victim);
        scr_puts(x, MSG_Y2, "WAS WRONG - DRAWS 6", COL_RED);
    }
}

void ui_game_over_screen(unsigned char human_won, unsigned char winner_idx) {
    scr_clear();
    if (human_won) {
        scr_puts(12, 8, "YOU WIN!", COL_GREEN);
        scr_puts(6, 10, "GREAT GAME, UNO CHAMPION.", COL_WHITE);
    } else {
        scr_puts(10, 8, "GAME OVER", COL_RED);
        scr_put(12, 10, 'C', COL_WHITE);
        scr_put(13, 10, 'P', COL_WHITE);
        scr_put(14, 10, 'U', COL_WHITE);
        scr_put(15, 10, '0' + winner_idx, COL_WHITE);
        scr_puts(17, 10, "WINS", COL_WHITE);
    }
    scr_puts(9, 20, "PRESS FIRE TO PLAY AGAIN", COL_CYAN);
}
