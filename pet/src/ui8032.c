#include <ascii_charmap.h>
#include "ui.h"
#include "petvid8032.h"

/* Same monochrome-only, bracketed-label approach as the 4032's ui.c (the
   PET has no color RAM at all, on any model) -- just spread across the
   full 80 columns instead of packed into 40, and needing only 2 hand
   rows instead of 4 since twice as many cards fit per row. */

#define TITLE_Y 0
#define OPP_Y 2
#define TABLE_Y 4
#define MSG_Y1 8
#define MSG_Y2 9
#define HAND_LABEL_Y 12
#define HAND_Y 13

/* Each hand entry is "[label:CV]" -- 6 characters wide. */
#define HAND_STRIDE 6
#define HAND_PER_ROW 12

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
    scr_puts(37, 3, "U N O");
    scr_puts(27, 5, "FOR THE COMMODORE PET 8032");
    scr_puts(25, 9, "1 PLAYER VS 3 COMPUTER PLAYERS");
    scr_puts(26, 11, "CRSR LEFT/RIGHT: PICK A CARD");
    scr_puts(24, 12, "SPACE OR RETURN: PLAY / CONFIRM");
    scr_puts(30, 13, "CRSR UP: DRAW A CARD");
    scr_puts(23, 14, "OR PRESS 1-9,0,A-J: PLAY THAT CARD");
    scr_puts(23, 17, "CARDS SHOW AS [LABEL:COLOR+VALUE]:");
    scr_puts(25, 18, "R=RED Y=YELLOW G=GREEN B=BLUE");
    scr_puts(28, 19, "S=SKIP V=REVERSE D=DRAW2");
    scr_puts(32, 20, "W=WILD F=WILD+4");

    scr_puts(30, 22, "PRESS FIRE TO START");
}

void ui_draw_frame(void) {
    scr_clear();
    scr_puts(33, TITLE_Y, "*** U N O ***");
}

void ui_draw_opponents(GameState *g) {
    unsigned char opp, x;
    scr_fill_rect(0, OPP_Y, COLS, 1, 32);
    for (opp = 1; opp <= 3; opp++) {
        x = 4 + (opp - 1) * 26;
        scr_put(x, OPP_Y, 'C');
        scr_put(x + 1, OPP_Y, 'P');
        scr_put(x + 2, OPP_Y, 'U');
        scr_put(x + 3, OPP_Y, (unsigned char)('0' + opp));
        scr_put(x + 4, OPP_Y, ':');
        scr_put_num(x + 6, OPP_Y, g->players[opp].count);
        scr_put(x + 9, OPP_Y, g->current_player == opp ? '<' : ' ');
    }
}

void ui_draw_table(GameState *g) {
    static const char *names[4] = {"RED", "YEL", "GRN", "BLU"};
    scr_fill_rect(0, TABLE_Y, COLS, 2, 32);
    scr_puts(1, TABLE_Y, "DRAW PILE:");
    scr_put_num(12, TABLE_Y, g->draw_count);

    scr_puts(40, TABLE_Y, "TOP:");
    scr_put(45, TABLE_Y, '[');
    scr_put(46, TABLE_Y, color_letter(g->top_card.color, g->top_color));
    scr_put(47, TABLE_Y, value_char(g->top_card));
    scr_put(48, TABLE_Y, ']');

    scr_puts(1, TABLE_Y + 1, "COLOR:");
    scr_puts(8, TABLE_Y + 1, names[g->top_color]);
    scr_puts(40, TABLE_Y + 1, g->direction > 0 ? "DIR ->" : "DIR <-");
}

/* Whether the previous call drew a second row, so we know whether that
   area needs clearing this time. */
static unsigned char prev_had_row2 = 0;

void ui_draw_hand(GameState *g, unsigned char cursor) {
    Player *p = &g->players[0];
    unsigned char i, x, y, shown, row_count, start_x;
    unsigned char has_row2 = (p->count > HAND_PER_ROW);

    scr_fill_rect(0, HAND_LABEL_Y, COLS, 1, 32);
    scr_puts(1, HAND_LABEL_Y, "YOUR HAND:");
    scr_put_num(12, HAND_LABEL_Y, p->count);

    scr_fill_rect(0, HAND_Y, COLS, 1, 32);
    if (has_row2 || prev_had_row2) {
        scr_fill_rect(0, HAND_Y + 1, COLS, 1, 32);
    }
    prev_had_row2 = has_row2;

    shown = (p->count > HAND_VISIBLE) ? HAND_VISIBLE : p->count;

    row_count = (shown > HAND_PER_ROW) ? HAND_PER_ROW : shown;
    start_x = (unsigned char)((COLS - row_count * HAND_STRIDE) / 2);

    for (i = 0; i < shown; i++) {
        if (i == HAND_PER_ROW) {
            row_count = shown - HAND_PER_ROW;
            start_x = (unsigned char)((COLS - row_count * HAND_STRIDE) / 2);
        }
        x = start_x + (i % HAND_PER_ROW) * HAND_STRIDE;
        y = (unsigned char)(HAND_Y + i / HAND_PER_ROW);
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
        x = 10 + i * 16;
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
        scr_puts(36, 8, "YOU WIN!");
        scr_puts(27, 10, "GREAT GAME, UNO CHAMPION.");
    } else {
        scr_puts(35, 8, "GAME OVER");
        scr_put(37, 10, 'C');
        scr_put(38, 10, 'P');
        scr_put(39, 10, 'U');
        scr_put(40, 10, (unsigned char)('0' + winner_idx));
        scr_puts(42, 10, "WINS");
    }
    scr_puts(28, 20, "PRESS FIRE TO PLAY AGAIN");
}
