#include <ascii_charmap.h>
#include "ui.h"
#include "petvid.h"

/* The PET is monochrome -- no color RAM at all -- so every card is shown
   as a bracketed [label:COLORLETTER+VALUE] instead of relying on a color
   change like the other ports do. 40x25 stock text, same layout style as
   the Plus/4 version. */

#define TITLE_Y 0
#define OPP_Y 2
#define TABLE_Y 4
#define MSG_Y1 7
#define MSG_Y2 8
#define HAND_LABEL_Y 10
#define HAND_Y 11

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
    scr_puts(17, 2, "U N O");
    scr_puts(8, 4, "FOR THE COMMODORE PET 4032");
    scr_puts(4, 8, "1 PLAYER VS 3 COMPUTER PLAYERS");
    scr_puts(6, 10, "CRSR LEFT/RIGHT: PICK A CARD");
    scr_puts(6, 11, "SPACE OR RETURN: PLAY / CONFIRM");
    scr_puts(6, 12, "CRSR UP: DRAW A CARD");
    scr_puts(2, 13, "OR PRESS 1-9,0,A-J: PLAY THAT CARD");
    scr_puts(2, 16, "CARDS SHOW AS [LABEL:COLOR+VALUE]:");
    scr_puts(2, 17, "R=RED Y=YELLOW G=GREEN B=BLUE");
    scr_puts(2, 18, "S=SKIP V=REVERSE D=DRAW2");
    scr_puts(2, 19, "W=WILD F=WILD+4");
    scr_puts(10, 22, "PRESS FIRE TO START");
}

void ui_draw_frame(void) {
    scr_clear();
    scr_puts(14, TITLE_Y, "*** U N O ***");
}

void ui_draw_opponents(GameState *g) {
    unsigned char opp, x;
    scr_fill_rect(0, OPP_Y, COLS, 1, 32);
    for (opp = 1; opp <= 3; opp++) {
        x = 1 + (opp - 1) * 13;
        scr_put(x, OPP_Y, 'C');
        scr_put(x + 1, OPP_Y, 'P');
        scr_put(x + 2, OPP_Y, 'U');
        scr_put(x + 3, OPP_Y, (unsigned char)('0' + opp));
        scr_put(x + 4, OPP_Y, ':');
        scr_put_num(x + 5, OPP_Y, g->players[opp].count);
        scr_put(x + 8, OPP_Y, g->current_player == opp ? '<' : ' ');
    }
}

void ui_draw_table(GameState *g) {
    static const char *names[4] = {"RED", "YEL", "GRN", "BLU"};
    scr_fill_rect(0, TABLE_Y, COLS, 2, 32);
    scr_puts(1, TABLE_Y, "DRAW PILE:");
    scr_put_num(12, TABLE_Y, g->draw_count);

    scr_puts(18, TABLE_Y, "TOP:");
    scr_put(23, TABLE_Y, '[');
    scr_put(24, TABLE_Y, color_letter(g->top_card.color, g->top_color));
    scr_put(25, TABLE_Y, value_char(g->top_card));
    scr_put(26, TABLE_Y, ']');

    scr_puts(1, TABLE_Y + 1, "COLOR:");
    scr_puts(8, TABLE_Y + 1, names[g->top_color]);
    scr_puts(18, TABLE_Y + 1, g->direction > 0 ? "DIR ->" : "DIR <-");
}

void ui_draw_hand(GameState *g, unsigned char cursor) {
    Player *p = &g->players[0];
    unsigned char i, x, y, shown;

    scr_fill_rect(0, HAND_LABEL_Y, COLS, 1, 32);
    scr_puts(1, HAND_LABEL_Y, "YOUR HAND:");
    scr_put_num(12, HAND_LABEL_Y, p->count);

    scr_fill_rect(0, HAND_Y, COLS, 4, 32);

    shown = (p->count > HAND_VISIBLE) ? HAND_VISIBLE : p->count;

    for (i = 0; i < shown; i++) {
        x = 1 + (i % 6) * 6;
        y = (unsigned char)(HAND_Y + i / 6);
        scr_put(x, y, i == cursor ? '[' : ' ');
        scr_put(x + 1, y, label_char(i));
        scr_put(x + 2, y, ':');
        scr_put(x + 3, y, color_letter(p->hand[i].color, NONE));
        scr_put(x + 4, y, value_char(p->hand[i]));
        scr_put(x + 5, y, i == cursor ? ']' : ' ');
    }
}

void ui_message(const char *line1, const char *line2) {
    scr_fill_rect(0, MSG_Y1, COLS, 2, 32);
    if (line1) scr_puts(1, MSG_Y1, line1);
    if (line2) scr_puts(1, MSG_Y2, line2);
}

void ui_draw_color_picker(unsigned char selected) {
    static const char *names[4] = {"RED", "YELLOW", "GREEN", "BLUE"};
    unsigned char i, x;

    scr_fill_rect(0, MSG_Y1, COLS, 2, 32);
    scr_puts(1, MSG_Y1, "CHOOSE A COLOR:");
    for (i = 0; i < 4; i++) {
        x = 2 + i * 9;
        scr_put(x, MSG_Y2, selected == i ? '[' : 32);
        scr_puts(x + 1, MSG_Y2, names[i]);
        scr_put(x + 1 + 6, MSG_Y2, selected == i ? ']' : 32);
    }
}

void ui_clear_color_picker(void) {
    scr_fill_rect(0, MSG_Y1, COLS, 2, 32);
}

static unsigned char player_label(unsigned char x, unsigned char row, unsigned char idx) {
    if (idx == 0) {
        scr_puts(x, row, "YOU");
        return x + 4;
    }
    scr_puts(x, row, "CPU");
    scr_put(x + 3, row, (unsigned char)('0' + idx));
    return x + 5;
}

void ui_event_skip(unsigned char idx) {
    unsigned char x;
    scr_fill_rect(0, MSG_Y1, COLS, 2, 32);
    x = player_label(1, MSG_Y1, idx);
    scr_puts(x, MSG_Y1, idx == 0 ? "LOSE A TURN (SKIPPED)" : "IS SKIPPED");
}

void ui_event_reverse(unsigned char idx) {
    (void)idx;
    scr_fill_rect(0, MSG_Y1, COLS, 2, 32);
    scr_puts(1, MSG_Y1, "REVERSE! PLAY ORDER FLIPPED");
}

void ui_event_draw(unsigned char idx, unsigned char count) {
    unsigned char x;
    scr_fill_rect(0, MSG_Y1, COLS, 2, 32);
    x = player_label(1, MSG_Y1, idx);
    scr_puts(x, MSG_Y1, idx == 0 ? "MUST DRAW" : "DRAWS");
    scr_put_num((unsigned char)(x + (idx == 0 ? 10 : 6)), MSG_Y1, count);
}

void ui_event_uno(unsigned char idx) {
    unsigned char x;
    scr_fill_rect(0, MSG_Y2, COLS, 1, 32);
    x = player_label(1, MSG_Y2, idx);
    scr_puts(x, MSG_Y2, "UNO! ONE CARD LEFT!");
}

void ui_event_invalid(void) {
    scr_fill_rect(0, MSG_Y2, COLS, 1, 32);
    scr_puts(1, MSG_Y2, "THAT CARD DOES NOT MATCH!");
}

void ui_event_drew_one(unsigned char idx) {
    unsigned char x;
    scr_fill_rect(0, MSG_Y1, COLS, 2, 32);
    x = player_label(1, MSG_Y1, idx);
    scr_puts(x, MSG_Y1, "NO LEGAL CARD, DREW ONE");
}

void ui_event_thinking(unsigned char idx) {
    unsigned char x;
    scr_fill_rect(0, MSG_Y1, COLS, 2, 32);
    x = player_label(1, MSG_Y1, idx);
    scr_puts(x, MSG_Y1, "IS THINKING...");
}

void ui_draw_challenge_prompt(unsigned char victim, unsigned char player, unsigned char selected_yes) {
    unsigned char x;
    scr_fill_rect(0, MSG_Y1, COLS, 2, 32);
    x = player_label(1, MSG_Y1, player);
    scr_puts(x, MSG_Y1, "PLAYED WILD DRAW FOUR");
    x = player_label(1, MSG_Y2, victim);
    scr_puts(x, MSG_Y2, "CHALLENGE?");
    x += 11;
    scr_put(x, MSG_Y2, selected_yes ? '[' : 32);
    scr_puts(x + 1, MSG_Y2, "YES");
    scr_put(x + 4, MSG_Y2, selected_yes ? ']' : 32);
    scr_put(x + 6, MSG_Y2, !selected_yes ? '[' : 32);
    scr_puts(x + 7, MSG_Y2, "NO");
    scr_put(x + 9, MSG_Y2, !selected_yes ? ']' : 32);
}

void ui_event_challenge_result(unsigned char victim, unsigned char player, unsigned char succeeded) {
    unsigned char x;
    scr_fill_rect(0, MSG_Y1, COLS, 2, 32);
    x = player_label(1, MSG_Y1, victim);
    scr_puts(x, MSG_Y1, "CHALLENGES!");
    if (succeeded) {
        x = player_label(1, MSG_Y2, player);
        scr_puts(x, MSG_Y2, "HAD A MATCH - DRAWS 4");
    } else {
        x = player_label(1, MSG_Y2, victim);
        scr_puts(x, MSG_Y2, "WAS WRONG - DRAWS 6");
    }
}

void ui_game_over_screen(unsigned char human_won, unsigned char winner_idx) {
    scr_clear();
    if (human_won) {
        scr_puts(12, 8, "YOU WIN!");
        scr_puts(6, 10, "GREAT GAME, UNO CHAMPION.");
    } else {
        scr_puts(10, 8, "GAME OVER");
        scr_put(12, 10, 'C');
        scr_put(13, 10, 'P');
        scr_put(14, 10, 'U');
        scr_put(15, 10, (unsigned char)('0' + winner_idx));
        scr_puts(17, 10, "WINS");
    }
    scr_puts(7, 20, "PRESS FIRE TO PLAY AGAIN");
}
