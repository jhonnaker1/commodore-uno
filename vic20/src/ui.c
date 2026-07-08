#include <ascii_charmap.h>
#include "ui.h"
#include "vic20io.h"

/* Color RAM was earlier thought not to visibly affect rendering at all,
   but that was a side effect of a since-fixed bug elsewhere (the whole
   screen was rendering white-on-white): every color value really did
   produce the same shade, because there was no visible shade at all.
   Confirmed working now (see vic20io.c), so cards get an actual suit
   color as well as the explicit letter -- the letter stays too, since
   it's still useful at a glance and costs nothing extra. Everything here
   is deliberately terse: the VIC-20's screen is only 22 columns wide (a
   hardware limit, not something memory expansion changes), so every
   string length is hand-counted against that. */

#define TITLE_Y 0
#define OPP_Y 2
#define TABLE_Y 4
#define MSG_Y1 8
#define MSG_Y2 9
#define HAND_LABEL_Y 11
#define HAND_Y 12

static const unsigned char suit_color[4] = {COL_RED, COL_YELLOW, COL_GREEN, COL_BLUE};

/* Wild cards sitting in hand have no assigned suit yet (only once played
   and a color is chosen), so they get a neutral color instead of
   indexing suit_color with COLOR_WILD. */
static unsigned char card_color(unsigned char color) {
    return (color == COLOR_WILD) ? COL_WHITE : suit_color[color];
}

static char color_letter(unsigned char color, unsigned char color_override) {
    if (color == COLOR_WILD) {
        if (color_override == NONE) return '?';
        color = color_override;
    }
    switch (color) {
        case COLOR_RED: return 'R';
        case COLOR_YELLOW: return 'Y';
        case COLOR_GREEN: return 'G';
        default: return 'B';
    }
}

/* '1'-'9', '0', then 'A'-'J' for slots 0-19 (matches the quick-play keys). */
static char label_char(unsigned char idx) {
    if (idx < 9) return (char)('1' + idx);
    if (idx == 9) return '0';
    return (char)('A' + (idx - 10));
}

static char value_char(Card c) {
    if (c.value <= 9) return (char)('0' + c.value);
    if (c.value == VAL_SKIP) return 'S';
    if (c.value == VAL_REVERSE) return 'V';
    if (c.value == VAL_DRAW2) return 'D';
    if (c.value == VAL_WILD) return 'W';
    return 'F';
}

void ui_title_screen(void) {
    scr_clear();
    scr_puts(8, 1, "U N O", COL_WHITE);
    scr_puts(3, 3, "COMMODORE VIC-20", COL_WHITE);
    scr_puts(3, 5, "VS 3 CPU PLAYERS", COL_WHITE);
    scr_puts(1, 7, "JOYSTICK PORT 1 OR:", COL_WHITE);
    scr_puts(1, 8, "CRSR L/R: PICK CARD", COL_WHITE);
    scr_puts(1, 9, "SPACE/RETURN: PLAY", COL_WHITE);
    scr_puts(1, 10, "CRSR UP: DRAW", COL_WHITE);
    scr_puts(1, 11, "1-9,0,A-J: PLAY CARD", COL_WHITE);
    scr_puts(1, 13, "CARDS:R/Y/G/B+VALUE", COL_WHITE);
    scr_puts(1, 14, "S=SKIP V=REV", COL_WHITE);
    scr_puts(1, 15, "D=DRAW2 W=WILD", COL_WHITE);
    scr_puts(1, 16, "F=WILD+4", COL_WHITE);
    scr_puts(5, 19, "FIRE=START", COL_WHITE);
}

void ui_draw_frame(void) {
    scr_clear();
    scr_puts(6, TITLE_Y, "* UNO *", COL_WHITE);
}

void ui_draw_opponents(GameState *g) {
    unsigned char opp, x;
    scr_fill_rect(0, OPP_Y, COLS, 1, 32, COL_BLACK);
    for (opp = 1; opp <= 3; opp++) {
        x = 1 + (opp - 1) * 6;
        scr_put(x, OPP_Y, 'P', COL_WHITE);
        scr_put(x + 1, OPP_Y, '0' + opp, COL_WHITE);
        scr_put(x + 2, OPP_Y, g->current_player == opp ? '>' : ':', COL_WHITE);
        scr_put_num(x + 3, OPP_Y, g->players[opp].count, COL_WHITE);
    }
}

void ui_draw_table(GameState *g) {
    static const char *names[4] = {"RED", "YEL", "GRN", "BLU"};
    scr_fill_rect(0, TABLE_Y, COLS, 3, 32, COL_BLACK);
    scr_puts(1, TABLE_Y, "DRAW:", COL_WHITE);
    scr_put_num(6, TABLE_Y, g->draw_count, COL_WHITE);

    scr_puts(1, TABLE_Y + 1, "TOP:[", COL_WHITE);
    scr_put(6, TABLE_Y + 1, color_letter(g->top_card.color, g->top_color), suit_color[g->top_color]);
    scr_put(7, TABLE_Y + 1, value_char(g->top_card), suit_color[g->top_color]);
    scr_puts(8, TABLE_Y + 1, "]", COL_WHITE);

    scr_puts(1, TABLE_Y + 2, "COL:", COL_WHITE);
    scr_puts(5, TABLE_Y + 2, names[g->top_color], suit_color[g->top_color]);
    scr_puts(9, TABLE_Y + 2, g->direction > 0 ? "DIR:>" : "DIR:<", COL_WHITE);
}

void ui_draw_hand(GameState *g, unsigned char cursor) {
    Player *p = &g->players[0];
    unsigned char i, x, y, shown;

    scr_fill_rect(0, HAND_LABEL_Y, COLS, 1, 32, COL_BLACK);
    scr_puts(1, HAND_LABEL_Y, "HAND:", COL_WHITE);
    scr_put_num(6, HAND_LABEL_Y, p->count, COL_WHITE);

    scr_fill_rect(0, HAND_Y, COLS, 5, 32, COL_BLACK);

    shown = (p->count > HAND_VISIBLE) ? HAND_VISIBLE : p->count;

    for (i = 0; i < shown; i++) {
        x = (i % 4) * 5;
        y = (unsigned char)(HAND_Y + i / 4);
        scr_put(x, y, i == cursor ? '[' : ' ', COL_WHITE);
        scr_put(x + 1, y, label_char(i), COL_WHITE);
        scr_put(x + 2, y, color_letter(p->hand[i].color, NONE), card_color(p->hand[i].color));
        scr_put(x + 3, y, value_char(p->hand[i]), card_color(p->hand[i].color));
        scr_put(x + 4, y, i == cursor ? ']' : ' ', COL_WHITE);
    }
}

void ui_message(const char *line1, const char *line2) {
    scr_fill_rect(0, MSG_Y1, COLS, 2, 32, COL_BLACK);
    if (line1) scr_puts(1, MSG_Y1, line1, COL_WHITE);
    if (line2) scr_puts(1, MSG_Y2, line2, COL_WHITE);
}

void ui_draw_color_picker(unsigned char selected) {
    static const char *names[4] = {"RED", "YEL", "GRN", "BLU"};
    unsigned char i, x;

    scr_fill_rect(0, MSG_Y1, COLS, 2, 32, COL_BLACK);
    scr_puts(1, MSG_Y1, "PICK A COLOR:", COL_WHITE);
    for (i = 0; i < 4; i++) {
        x = 1 + i * 5;
        scr_put(x, MSG_Y2, selected == i ? '[' : ' ', COL_WHITE);
        scr_puts(x + 1, MSG_Y2, names[i], suit_color[i]);
        scr_put(x + 4, MSG_Y2, selected == i ? ']' : ' ', COL_WHITE);
    }
}

void ui_clear_color_picker(void) {
    scr_fill_rect(0, MSG_Y1, COLS, 2, 32, COL_BLACK);
}

static unsigned char player_label(unsigned char x, unsigned char row, unsigned char idx) {
    if (idx == 0) {
        scr_puts(x, row, "YOU", COL_WHITE);
        return x + 4;
    }
    scr_puts(x, row, "CPU", COL_WHITE);
    scr_put(x + 3, row, '0' + idx, COL_WHITE);
    return x + 5;
}

void ui_event_skip(unsigned char idx) {
    unsigned char x;
    scr_fill_rect(0, MSG_Y1, COLS, 2, 32, COL_BLACK);
    x = player_label(1, MSG_Y1, idx);
    scr_puts(x, MSG_Y1, "SKIPPED", COL_WHITE);
}

void ui_event_reverse(unsigned char idx) {
    (void)idx;
    scr_fill_rect(0, MSG_Y1, COLS, 2, 32, COL_BLACK);
    scr_puts(1, MSG_Y1, "REVERSED!", COL_WHITE);
}

void ui_event_draw(unsigned char idx, unsigned char count) {
    unsigned char x;
    scr_fill_rect(0, MSG_Y1, COLS, 2, 32, COL_BLACK);
    x = player_label(1, MSG_Y1, idx);
    scr_puts(x, MSG_Y1, "DRAW", COL_WHITE);
    scr_put_num(x + 5, MSG_Y1, count, COL_WHITE);
}

void ui_event_uno(unsigned char idx) {
    unsigned char x;
    scr_fill_rect(0, MSG_Y2, COLS, 1, 32, COL_BLACK);
    x = player_label(1, MSG_Y2, idx);
    scr_puts(x, MSG_Y2, "UNO!", COL_WHITE);
}

void ui_event_invalid(void) {
    scr_fill_rect(0, MSG_Y2, COLS, 1, 32, COL_BLACK);
    scr_puts(1, MSG_Y2, "NO MATCH!", COL_WHITE);
}

void ui_event_drew_one(unsigned char idx) {
    unsigned char x;
    scr_fill_rect(0, MSG_Y1, COLS, 2, 32, COL_BLACK);
    x = player_label(1, MSG_Y1, idx);
    scr_puts(x, MSG_Y1, "DREW 1", COL_WHITE);
}

void ui_event_thinking(unsigned char idx) {
    unsigned char x;
    scr_fill_rect(0, MSG_Y1, COLS, 2, 32, COL_BLACK);
    x = player_label(1, MSG_Y1, idx);
    scr_puts(x, MSG_Y1, "THINKS", COL_WHITE);
}

void ui_draw_challenge_prompt(unsigned char victim, unsigned char player, unsigned char selected_yes) {
    unsigned char x;
    scr_fill_rect(0, MSG_Y1, COLS, 2, 32, COL_BLACK);
    x = player_label(1, MSG_Y1, player);
    scr_puts(x, MSG_Y1, "WILD+4", COL_WHITE);
    x = player_label(1, MSG_Y2, victim);
    scr_puts(x, MSG_Y2, "CHAL?", COL_WHITE);
    scr_put(x + 6, MSG_Y2, selected_yes ? '[' : ' ', COL_WHITE);
    scr_puts(x + 7, MSG_Y2, "YES", COL_WHITE);
    scr_put(x + 10, MSG_Y2, selected_yes ? ']' : ' ', COL_WHITE);
    scr_put(x + 12, MSG_Y2, !selected_yes ? '[' : ' ', COL_WHITE);
    scr_puts(x + 13, MSG_Y2, "NO", COL_WHITE);
    scr_put(x + 15, MSG_Y2, !selected_yes ? ']' : ' ', COL_WHITE);
}

void ui_event_challenge_result(unsigned char victim, unsigned char player, unsigned char succeeded) {
    unsigned char x;
    scr_fill_rect(0, MSG_Y1, COLS, 2, 32, COL_BLACK);
    x = player_label(1, MSG_Y1, victim);
    scr_puts(x, MSG_Y1, "CHALLNG!", COL_WHITE);
    if (succeeded) {
        x = player_label(1, MSG_Y2, player);
        scr_puts(x, MSG_Y2, "DRAWS 4", COL_WHITE);
    } else {
        x = player_label(1, MSG_Y2, victim);
        scr_puts(x, MSG_Y2, "WRONG-6", COL_WHITE);
    }
}

void ui_game_over_screen(unsigned char human_won, unsigned char winner_idx) {
    scr_clear();
    if (human_won) {
        scr_puts(6, 8, "YOU WIN!", COL_WHITE);
        scr_puts(2, 10, "UNO CHAMPION!", COL_WHITE);
    } else {
        scr_puts(5, 8, "GAME OVER", COL_WHITE);
        scr_put(6, 10, 'C', COL_WHITE);
        scr_put(7, 10, 'P', COL_WHITE);
        scr_put(8, 10, 'U', COL_WHITE);
        scr_put(9, 10, '0' + winner_idx, COL_WHITE);
        scr_puts(11, 10, "WINS", COL_WHITE);
    }
    scr_puts(2, 15, "FIRE:PLAY AGAIN", COL_WHITE);
}
