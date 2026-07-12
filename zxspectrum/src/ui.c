#include "ui.h"
#include "zxvid.h"

/* Cards are still shown as bracketed [label:COLORLETTER+VALUE] (same
   trick as the VIC-20/PET/Atari ports -- no redefinable character
   generator here either, so no custom card-shaped tiles like the C64
   port), but the bracket/letter/value are drawn in the card's real suit
   colour now, not a single ink (see zxvid.h for why that's actually free
   at the text level despite the Spectrum's attribute-clash reputation).
   The cursor position uses COL_SELECTED instead of the card's own
   colour, so it stays visible regardless of which suit is under it --
   same role INVERSE played in the single-colour version. Screen is 32
   columns, narrower than every other port here, so layout spacing below
   is Spectrum-specific (5 cards/row instead of 6, tighter opponent/
   color-picker spacing). */

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

/* Maps a card's colour (with wild-card override once chosen) to the
   COL_* ink value it should actually be drawn in. */
static unsigned char card_color(unsigned char color, unsigned char color_override) {
    if (color == COLOR_WILD) {
        if (color_override == NONE) return COL_WILD;
        color = color_override;
    }
    switch (color) {
        case COLOR_RED: return COL_RED;
        case COLOR_YELLOW: return COL_YELLOW;
        case COLOR_GREEN: return COL_GREEN;
        default: return COL_BLUE;
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
    scr_puts(13, 1, "U N O", COL_NORMAL);
    scr_puts(4, 3, "FOR THE ZX SPECTRUM", COL_NORMAL);
    scr_puts(1, 6, "1 PLAYER VS 3 COMPUTERS", COL_NORMAL);
    scr_puts(1, 8, "O/P: PICK A CARD", COL_NORMAL);
    scr_puts(1, 9, "SPACE/ENTER: PLAY OR CONFIRM", COL_NORMAL);
    scr_puts(1, 10, "Q: DRAW A CARD", COL_NORMAL);
    scr_puts(0, 11, "OR PRESS 1-9,0,A-J FOR THAT CARD", COL_NORMAL);
    scr_puts(0, 14, "CARDS SHOW AS [LABEL:COLOR+VAL]", COL_NORMAL);
    scr_puts(0, 15, "IN THEIR REAL COLOR:", COL_NORMAL);
    scr_puts(0, 16, "R", COL_RED);
    scr_puts(1, 16, "=RED ", COL_NORMAL);
    scr_puts(6, 16, "Y", COL_YELLOW);
    scr_puts(7, 16, "=YELLOW ", COL_NORMAL);
    scr_puts(15, 16, "G", COL_GREEN);
    scr_puts(16, 16, "=GREEN ", COL_NORMAL);
    scr_puts(23, 16, "B", COL_BLUE);
    scr_puts(24, 16, "=BLUE", COL_NORMAL);
    scr_puts(0, 17, "S=SKIP V=REVERSE D=DRAW2", COL_NORMAL);
    scr_puts(0, 18, "W", COL_WILD);
    scr_puts(1, 18, "=WILD F=WILD+4", COL_NORMAL);
    scr_puts(4, 20, "PRESS SPACE TO START", COL_NORMAL);
}

void ui_draw_frame(void) {
    scr_clear();
    scr_puts(14, TITLE_Y, "*** U N O ***", COL_NORMAL);
}

void ui_draw_opponents(GameState *g) {
    unsigned char opp, x;
    scr_fill_rect(0, OPP_Y, COLS, 1);
    for (opp = 1; opp <= 3; opp++) {
        x = 1 + (opp - 1) * 10;
        scr_put(x, OPP_Y, 'C', COL_NORMAL);
        scr_put(x + 1, OPP_Y, 'P', COL_NORMAL);
        scr_put(x + 2, OPP_Y, 'U', COL_NORMAL);
        scr_put(x + 3, OPP_Y, (char)('0' + opp), COL_NORMAL);
        scr_put(x + 4, OPP_Y, ':', COL_NORMAL);
        scr_put_num(x + 5, OPP_Y, g->players[opp].count);
        scr_put(x + 8, OPP_Y, g->current_player == opp ? '<' : ' ', COL_NORMAL);
    }
}

void ui_draw_table(GameState *g) {
    static const char *names[4] = {"RED", "YEL", "GRN", "BLU"};
    unsigned char col = card_color(g->top_card.color, g->top_color);
    scr_fill_rect(0, TABLE_Y, COLS, 2);
    scr_puts(1, TABLE_Y, "DRAW PILE:", COL_NORMAL);
    scr_put_num(12, TABLE_Y, g->draw_count);

    scr_puts(18, TABLE_Y, "TOP:", COL_NORMAL);
    scr_put(23, TABLE_Y, '[', col);
    scr_put(24, TABLE_Y, color_letter(g->top_card.color, g->top_color), col);
    scr_put(25, TABLE_Y, value_char(g->top_card), col);
    scr_put(26, TABLE_Y, ']', col);

    scr_puts(1, TABLE_Y + 1, "COLOR:", COL_NORMAL);
    scr_puts(8, TABLE_Y + 1, names[g->top_color], col);
    scr_puts(18, TABLE_Y + 1, g->direction > 0 ? "DIR ->" : "DIR <-", COL_NORMAL);
}

void ui_draw_hand(GameState *g, unsigned char cursor) {
    Player *p = &g->players[0];
    unsigned char i, x, y, shown, col;

    scr_fill_rect(0, HAND_LABEL_Y, COLS, 1);
    scr_puts(1, HAND_LABEL_Y, "YOUR HAND:", COL_NORMAL);
    scr_put_num(12, HAND_LABEL_Y, p->count);

    scr_fill_rect(0, HAND_Y, COLS, 4);

    shown = (p->count > HAND_VISIBLE) ? HAND_VISIBLE : p->count;

    for (i = 0; i < shown; i++) {
        col = (i == cursor) ? COL_SELECTED : card_color(p->hand[i].color, NONE);
        x = 1 + (i % 5) * 6;
        y = (unsigned char)(HAND_Y + i / 5);
        scr_put(x, y, '[', col);
        scr_put(x + 1, y, label_char(i), col);
        scr_put(x + 2, y, ':', col);
        scr_put(x + 3, y, color_letter(p->hand[i].color, NONE), col);
        scr_put(x + 4, y, value_char(p->hand[i]), col);
        scr_put(x + 5, y, ']', col);
    }
}

void ui_message(const char *line1, const char *line2) {
    scr_fill_rect(0, MSG_Y1, COLS, 2);
    if (line1) scr_puts(1, MSG_Y1, line1, COL_NORMAL);
    if (line2) scr_puts(1, MSG_Y2, line2, COL_NORMAL);
}

void ui_draw_color_picker(unsigned char selected) {
    static const char *names[4] = {"RED", "YELLOW", "GREEN", "BLUE"};
    static const unsigned char cols[4] = {COL_RED, COL_YELLOW, COL_GREEN, COL_BLUE};
    static const unsigned char lens[4] = {3, 6, 5, 4};
    unsigned char i, x;

    scr_fill_rect(0, MSG_Y1, COLS, 2);
    scr_puts(1, MSG_Y1, "CHOOSE A COLOR:", COL_NORMAL);
    x = 1;
    for (i = 0; i < 4; i++) {
        unsigned char sel = (selected == i);
        scr_put(x, MSG_Y2, sel ? '[' : ' ', COL_NORMAL);
        scr_puts(x + 1, MSG_Y2, names[i], sel ? COL_SELECTED : cols[i]);
        scr_put((unsigned char)(x + 1 + lens[i]), MSG_Y2, sel ? ']' : ' ', COL_NORMAL);
        x = (unsigned char)(x + 1 + lens[i] + 2);
    }
}

void ui_clear_color_picker(void) {
    scr_fill_rect(0, MSG_Y1, COLS, 2);
}

static unsigned char player_label(unsigned char x, unsigned char row, unsigned char idx) {
    if (idx == 0) {
        scr_puts(x, row, "YOU", COL_NORMAL);
        return x + 4;
    }
    scr_puts(x, row, "CPU", COL_NORMAL);
    scr_put(x + 3, row, (char)('0' + idx), COL_NORMAL);
    return x + 5;
}

void ui_event_skip(unsigned char idx) {
    unsigned char x;
    scr_fill_rect(0, MSG_Y1, COLS, 2);
    x = player_label(1, MSG_Y1, idx);
    scr_puts(x, MSG_Y1, idx == 0 ? "LOSE A TURN (SKIPPED)" : "IS SKIPPED", COL_NORMAL);
}

void ui_event_reverse(unsigned char idx) {
    (void)idx;
    scr_fill_rect(0, MSG_Y1, COLS, 2);
    scr_puts(1, MSG_Y1, "REVERSE! PLAY ORDER FLIPPED", COL_NORMAL);
}

void ui_event_draw(unsigned char idx, unsigned char count) {
    unsigned char x;
    scr_fill_rect(0, MSG_Y1, COLS, 2);
    x = player_label(1, MSG_Y1, idx);
    scr_puts(x, MSG_Y1, idx == 0 ? "MUST DRAW" : "DRAWS", COL_NORMAL);
    scr_put_num((unsigned char)(x + (idx == 0 ? 10 : 6)), MSG_Y1, count);
}

void ui_event_uno(unsigned char idx) {
    unsigned char x;
    scr_fill_rect(0, MSG_Y2, COLS, 1);
    x = player_label(1, MSG_Y2, idx);
    scr_puts(x, MSG_Y2, "UNO! ONE CARD LEFT!", COL_YELLOW);
}

void ui_event_invalid(void) {
    scr_fill_rect(0, MSG_Y2, COLS, 1);
    scr_puts(1, MSG_Y2, "THAT CARD DOES NOT MATCH!", COL_RED);
}

void ui_event_drew_one(unsigned char idx) {
    unsigned char x;
    scr_fill_rect(0, MSG_Y1, COLS, 2);
    x = player_label(1, MSG_Y1, idx);
    scr_puts(x, MSG_Y1, "NO LEGAL CARD, DREW ONE", COL_NORMAL);
}

void ui_event_thinking(unsigned char idx) {
    unsigned char x;
    scr_fill_rect(0, MSG_Y1, COLS, 2);
    x = player_label(1, MSG_Y1, idx);
    scr_puts(x, MSG_Y1, "IS THINKING...", COL_NORMAL);
}

void ui_draw_challenge_prompt(unsigned char victim, unsigned char player, unsigned char selected_yes) {
    unsigned char x;
    scr_fill_rect(0, MSG_Y1, COLS, 2);
    x = player_label(1, MSG_Y1, player);
    scr_puts(x, MSG_Y1, "PLAYED WILD DRAW FOUR", COL_NORMAL);
    x = player_label(1, MSG_Y2, victim);
    scr_puts(x, MSG_Y2, "CHALLENGE?", COL_NORMAL);
    x += 11;
    scr_put(x, MSG_Y2, selected_yes ? '[' : ' ', COL_NORMAL);
    scr_puts(x + 1, MSG_Y2, "YES", selected_yes ? COL_SELECTED : COL_NORMAL);
    scr_put(x + 4, MSG_Y2, selected_yes ? ']' : ' ', COL_NORMAL);
    scr_put(x + 6, MSG_Y2, !selected_yes ? '[' : ' ', COL_NORMAL);
    scr_puts(x + 7, MSG_Y2, "NO", !selected_yes ? COL_SELECTED : COL_NORMAL);
    scr_put(x + 9, MSG_Y2, !selected_yes ? ']' : ' ', COL_NORMAL);
}

void ui_event_challenge_result(unsigned char victim, unsigned char player, unsigned char succeeded) {
    unsigned char x;
    scr_fill_rect(0, MSG_Y1, COLS, 2);
    x = player_label(1, MSG_Y1, victim);
    scr_puts(x, MSG_Y1, "CHALLENGES!", COL_NORMAL);
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
        scr_puts(12, 8, "YOU WIN!", COL_GREEN);
        scr_puts(6, 10, "GREAT GAME, UNO CHAMPION.", COL_NORMAL);
    } else {
        scr_puts(10, 8, "GAME OVER", COL_RED);
        scr_put(12, 10, 'C', COL_NORMAL);
        scr_put(13, 10, 'P', COL_NORMAL);
        scr_put(14, 10, 'U', COL_NORMAL);
        scr_put(15, 10, (char)('0' + winner_idx), COL_NORMAL);
        scr_puts(17, 10, "WINS", COL_NORMAL);
    }
    scr_puts(7, 20, "PRESS FIRE TO PLAY AGAIN", COL_NORMAL);
}
