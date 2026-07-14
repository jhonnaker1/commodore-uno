#include "ui.h"
#include "vbxevid.h"

/* VBXE's text overlay mode gives real per-character color (128
   foreground colors from a 1024-color palette -- see vbxevid.c), so
   unlike atari/src/ui.c (stock Atari text mode, one fg/bg for the
   whole screen), cards render in their actual suit color instead of
   relying on a bracketed color-letter alone. The bracket/letter/value
   scheme is kept anyway for consistency with the rest of this repo's
   real-color ports (VIC-20, CoCo3, ZX Spectrum) -- it costs nothing
   once color is available and stays instantly readable. The cursor
   position uses a dedicated highlight color instead of the card's
   own, so it's visible regardless of suit. 64 columns (VBXE text
   mode's narrowest option -- see vbxevid.h) fits 8 cards/row instead
   of the 40-column stock port's 6. */

#define TITLE_Y 0
#define OPP_Y 2
#define TABLE_Y 4
#define MSG_Y1 7
#define MSG_Y2 8
#define HAND_LABEL_Y 10
#define HAND_Y 11
#define CARDS_PER_ROW 8
#define COL_SELECTED COL_WHITE
#define COL_WILD COL_MAGENTA

static unsigned char suit_color(unsigned char color) {
    switch (color) {
        case COLOR_RED: return COL_RED;
        case COLOR_YELLOW: return COL_YELLOW;
        case COLOR_GREEN: return COL_GREEN;
        default: return COL_BLUE;
    }
}

static unsigned char card_color(unsigned char color, unsigned char color_override) {
    if (color == COLOR_WILD) {
        if (color_override == NONE) return COL_WILD;
        return suit_color(color_override);
    }
    return suit_color(color);
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
    scr_puts(29, 2, "U N O", COL_WHITE);
    scr_puts(20, 4, "FOR THE ATARI 800XL + VBXE", COL_WHITE);
    scr_puts(16, 8, "1 PLAYER VS 3 COMPUTER PLAYERS", COL_WHITE);
    scr_puts(18, 10, "JOYSTICK PORT 1, OR:", COL_WHITE);
    scr_puts(15, 11, "CRSR LEFT/RIGHT: PICK A CARD", COL_WHITE);
    scr_puts(15, 12, "SPACE OR RETURN: PLAY / CONFIRM", COL_WHITE);
    scr_puts(15, 13, "CRSR UP: DRAW A CARD", COL_WHITE);
    scr_puts(11, 14, "OR PRESS 1-9,0,A-J: PLAY THAT CARD", COL_WHITE);
    scr_puts(11, 17, "CARDS SHOW AS [LABEL:COLOR+VALUE]", COL_WHITE);
    scr_puts(11, 18, "IN THEIR REAL COLOR:", COL_WHITE);
    scr_puts(11, 19, "R", COL_RED);
    scr_puts(12, 19, "=RED  ", COL_WHITE);
    scr_puts(18, 19, "Y", COL_YELLOW);
    scr_puts(19, 19, "=YELLOW  ", COL_WHITE);
    scr_puts(28, 19, "G", COL_GREEN);
    scr_puts(29, 19, "=GREEN  ", COL_WHITE);
    scr_puts(37, 19, "B", COL_BLUE);
    scr_puts(38, 19, "=BLUE", COL_WHITE);
    scr_puts(11, 20, "S=SKIP V=REVERSE D=DRAW2", COL_WHITE);
    scr_puts(11, 21, "W", COL_WILD);
    scr_puts(12, 21, "=WILD F=WILD+4", COL_WHITE);
    scr_puts(22, 23, "PRESS FIRE TO START", COL_WHITE);
}

void ui_draw_frame(void) {
    scr_clear();
    scr_puts(26, TITLE_Y, "*** U N O ***", COL_WHITE);
}

void ui_draw_opponents(GameState *g) {
    unsigned char opp, x;
    scr_fill_rect(0, OPP_Y, COLS, 1, ' ', COL_BLACK);
    for (opp = 1; opp <= 3; opp++) {
        x = 2 + (opp - 1) * 20;
        scr_put(x, OPP_Y, 'C', COL_WHITE);
        scr_put(x + 1, OPP_Y, 'P', COL_WHITE);
        scr_put(x + 2, OPP_Y, 'U', COL_WHITE);
        scr_put(x + 3, OPP_Y, (unsigned char)('0' + opp), COL_WHITE);
        scr_put(x + 4, OPP_Y, ':', COL_WHITE);
        scr_put_num(x + 5, OPP_Y, g->players[opp].count, COL_WHITE);
        scr_put(x + 8, OPP_Y, g->current_player == opp ? '<' : ' ', COL_YELLOW);
    }
}

void ui_draw_table(GameState *g) {
    static const char *names[4] = {"RED", "YEL", "GRN", "BLU"};
    unsigned char col = card_color(g->top_card.color, g->top_color);
    scr_fill_rect(0, TABLE_Y, COLS, 2, ' ', COL_BLACK);
    scr_puts(1, TABLE_Y, "DRAW PILE:", COL_WHITE);
    scr_put_num(12, TABLE_Y, g->draw_count, COL_WHITE);

    scr_puts(20, TABLE_Y, "TOP:", COL_WHITE);
    scr_put(25, TABLE_Y, '[', col);
    scr_put(26, TABLE_Y, color_letter(g->top_card.color, g->top_color), col);
    scr_put(27, TABLE_Y, value_char(g->top_card), col);
    scr_put(28, TABLE_Y, ']', col);

    scr_puts(1, TABLE_Y + 1, "COLOR:", COL_WHITE);
    scr_puts(8, TABLE_Y + 1, names[g->top_color], col);
    scr_puts(20, TABLE_Y + 1, g->direction > 0 ? "DIR ->" : "DIR <-", COL_WHITE);
}

void ui_draw_hand(GameState *g, unsigned char cursor) {
    Player *p = &g->players[0];
    unsigned char i, x, y, shown, col;

    scr_fill_rect(0, HAND_LABEL_Y, COLS, 1, ' ', COL_BLACK);
    scr_puts(1, HAND_LABEL_Y, "YOUR HAND:", COL_WHITE);
    scr_put_num(12, HAND_LABEL_Y, p->count, COL_WHITE);

    scr_fill_rect(0, HAND_Y, COLS, 3, ' ', COL_BLACK);

    shown = (p->count > HAND_VISIBLE) ? HAND_VISIBLE : p->count;

    for (i = 0; i < shown; i++) {
        col = (i == cursor) ? COL_SELECTED : card_color(p->hand[i].color, NONE);
        x = 1 + (i % CARDS_PER_ROW) * 6;
        y = (unsigned char)(HAND_Y + i / CARDS_PER_ROW);
        scr_put(x, y, '[', col);
        scr_put(x + 1, y, (unsigned char)label_char(i), col);
        scr_put(x + 2, y, ':', col);
        scr_put(x + 3, y, (unsigned char)color_letter(p->hand[i].color, NONE), col);
        scr_put(x + 4, y, (unsigned char)value_char(p->hand[i]), col);
        scr_put(x + 5, y, ']', col);
    }
}

void ui_message(const char *line1, const char *line2) {
    scr_fill_rect(0, MSG_Y1, COLS, 2, ' ', COL_BLACK);
    if (line1) scr_puts(1, MSG_Y1, line1, COL_WHITE);
    if (line2) scr_puts(1, MSG_Y2, line2, COL_WHITE);
}

void ui_draw_color_picker(unsigned char selected) {
    static const char *names[4] = {"RED", "YELLOW", "GREEN", "BLUE"};
    static const unsigned char cols[4] = {COL_RED, COL_YELLOW, COL_GREEN, COL_BLUE};
    unsigned char i, x;

    scr_fill_rect(0, MSG_Y1, COLS, 2, ' ', COL_BLACK);
    scr_puts(1, MSG_Y1, "CHOOSE A COLOR:", COL_WHITE);
    for (i = 0; i < 4; i++) {
        unsigned char sel = (selected == i);
        x = 2 + i * 9;
        scr_put(x, MSG_Y2, sel ? '[' : ' ', COL_WHITE);
        scr_puts(x + 1, MSG_Y2, names[i], sel ? COL_SELECTED : cols[i]);
        scr_put(x + 1 + 6, MSG_Y2, sel ? ']' : ' ', COL_WHITE);
    }
}

void ui_clear_color_picker(void) {
    scr_fill_rect(0, MSG_Y1, COLS, 2, ' ', COL_BLACK);
}

static unsigned char player_label(unsigned char x, unsigned char row, unsigned char idx) {
    if (idx == 0) {
        scr_puts(x, row, "YOU", COL_WHITE);
        return x + 4;
    }
    scr_puts(x, row, "CPU", COL_WHITE);
    scr_put(x + 3, row, (unsigned char)('0' + idx), COL_WHITE);
    return x + 5;
}

void ui_event_skip(unsigned char idx) {
    unsigned char x;
    scr_fill_rect(0, MSG_Y1, COLS, 2, ' ', COL_BLACK);
    x = player_label(1, MSG_Y1, idx);
    scr_puts(x, MSG_Y1, idx == 0 ? "LOSE A TURN (SKIPPED)" : "IS SKIPPED", COL_WHITE);
}

void ui_event_reverse(unsigned char idx) {
    (void)idx;
    scr_fill_rect(0, MSG_Y1, COLS, 2, ' ', COL_BLACK);
    scr_puts(1, MSG_Y1, "REVERSE! PLAY ORDER FLIPPED", COL_WHITE);
}

void ui_event_draw(unsigned char idx, unsigned char count) {
    unsigned char x;
    scr_fill_rect(0, MSG_Y1, COLS, 2, ' ', COL_BLACK);
    x = player_label(1, MSG_Y1, idx);
    scr_puts(x, MSG_Y1, idx == 0 ? "MUST DRAW" : "DRAWS", COL_WHITE);
    scr_put_num((unsigned char)(x + (idx == 0 ? 10 : 6)), MSG_Y1, count, COL_WHITE);
}

void ui_event_uno(unsigned char idx) {
    unsigned char x;
    scr_fill_rect(0, MSG_Y2, COLS, 1, ' ', COL_BLACK);
    x = player_label(1, MSG_Y2, idx);
    scr_puts(x, MSG_Y2, "UNO! ONE CARD LEFT!", COL_YELLOW);
}

void ui_event_invalid(void) {
    scr_fill_rect(0, MSG_Y2, COLS, 1, ' ', COL_BLACK);
    scr_puts(1, MSG_Y2, "THAT CARD DOES NOT MATCH!", COL_RED);
}

void ui_event_drew_one(unsigned char idx) {
    unsigned char x;
    scr_fill_rect(0, MSG_Y1, COLS, 2, ' ', COL_BLACK);
    x = player_label(1, MSG_Y1, idx);
    scr_puts(x, MSG_Y1, "NO LEGAL CARD, DREW ONE", COL_WHITE);
}

void ui_event_thinking(unsigned char idx) {
    unsigned char x;
    scr_fill_rect(0, MSG_Y1, COLS, 2, ' ', COL_BLACK);
    x = player_label(1, MSG_Y1, idx);
    scr_puts(x, MSG_Y1, "IS THINKING...", COL_WHITE);
}

void ui_draw_challenge_prompt(unsigned char victim, unsigned char player, unsigned char selected_yes) {
    unsigned char x;
    scr_fill_rect(0, MSG_Y1, COLS, 2, ' ', COL_BLACK);
    x = player_label(1, MSG_Y1, player);
    scr_puts(x, MSG_Y1, "PLAYED WILD DRAW FOUR", COL_WHITE);
    x = player_label(1, MSG_Y2, victim);
    scr_puts(x, MSG_Y2, "CHALLENGE?", COL_WHITE);
    x += 11;
    scr_put(x, MSG_Y2, selected_yes ? '[' : ' ', COL_WHITE);
    scr_puts(x + 1, MSG_Y2, "YES", selected_yes ? COL_SELECTED : COL_WHITE);
    scr_put(x + 4, MSG_Y2, selected_yes ? ']' : ' ', COL_WHITE);
    scr_put(x + 6, MSG_Y2, !selected_yes ? '[' : ' ', COL_WHITE);
    scr_puts(x + 7, MSG_Y2, "NO", !selected_yes ? COL_SELECTED : COL_WHITE);
    scr_put(x + 9, MSG_Y2, !selected_yes ? ']' : ' ', COL_WHITE);
}

void ui_event_challenge_result(unsigned char victim, unsigned char player, unsigned char succeeded) {
    unsigned char x;
    scr_fill_rect(0, MSG_Y1, COLS, 2, ' ', COL_BLACK);
    x = player_label(1, MSG_Y1, victim);
    scr_puts(x, MSG_Y1, "CHALLENGES!", COL_WHITE);
    if (succeeded) {
        x = player_label(1, MSG_Y2, player);
        scr_puts(x, MSG_Y2, "HAD A MATCH - DRAWS 4", COL_RED);
    } else {
        x = player_label(1, MSG_Y2, victim);
        scr_puts(x, MSG_Y2, "WAS WRONG - DRAWS 6", COL_RED);
    }
}

void ui_game_over_screen(unsigned char human_won, unsigned char winner_idx) {
    scr_clear();
    if (human_won) {
        scr_puts(28, 8, "YOU WIN!", COL_GREEN);
        scr_puts(19, 10, "GREAT GAME, UNO CHAMPION.", COL_WHITE);
    } else {
        scr_puts(27, 8, "GAME OVER", COL_RED);
        scr_put(29, 10, 'C', COL_WHITE);
        scr_put(30, 10, 'P', COL_WHITE);
        scr_put(31, 10, 'U', COL_WHITE);
        scr_put(32, 10, (unsigned char)('0' + winner_idx), COL_WHITE);
        scr_puts(34, 10, "WINS", COL_WHITE);
    }
    scr_puts(20, 20, "PRESS FIRE TO PLAY AGAIN", COL_WHITE);
}
