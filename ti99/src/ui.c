#include "ui.h"
#include "tivid.h"

/* 32x24, the same width as the ZX Spectrum port, but the cards are solid
   colour tiles rather than coloured letters -- see tivid.h for how character
   codes are traded for colours to make that possible on a machine with no
   per-cell colour.

   A hand card is drawn as a white slot label followed by a four-cell tile,
   "1[R5]", so six cards fit per row. The label sits *outside* the tile
   deliberately: the coloured ranges hold only 23 glyphs, and spending ten of
   them on A-J slot labels would not leave room for the card values. */

#define TITLE_Y 0
#define OPP_Y 2
#define TABLE_Y 4
#define MSG_Y1 7
#define MSG_Y2 8
#define HAND_LABEL_Y 10
#define HAND_Y 11

#define CARDS_PER_ROW 6
#define CARD_W 5

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

/* Maps a card's colour (with wild-card override once chosen) to the COL_*
   character range it should be drawn from. */
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

/* The four-cell card face: [ suit value ]. Every cell comes from the same
   colour range, so the whole tile is one solid block of the card's colour. */
static void draw_tile(unsigned char x, unsigned char y, char suit, char val,
                      unsigned char col) {
    scr_put(x, y, '[', col);
    scr_put((unsigned char)(x + 1), y, suit, col);
    scr_put((unsigned char)(x + 2), y, val, col);
    scr_put((unsigned char)(x + 3), y, ']', col);
}

void ui_title_screen(void) {
    scr_clear();
    scr_puts(13, 1, "U N O", COL_NORMAL);
    scr_puts(6, 3, "FOR THE TI-99/4A", COL_NORMAL);
    scr_puts(4, 5, "1 PLAYER VS 3 COMPUTERS", COL_NORMAL);

    scr_puts(1, 8, ", .  PICK A CARD", COL_NORMAL);
    scr_puts(1, 9, "SPACE PLAY OR CONFIRM", COL_NORMAL);
    scr_puts(1, 10, "U     DRAW A CARD", COL_NORMAL);
    scr_puts(1, 11, "1-9,0,A-J JUMP TO A CARD", COL_NORMAL);

    scr_puts(1, 14, "CARDS ARE COLOR TILES:", COL_NORMAL);
    draw_tile(1, 16, 'R', '5', COL_RED);
    scr_puts(6, 16, "RED", COL_NORMAL);
    draw_tile(11, 16, 'Y', 'S', COL_YELLOW);
    scr_puts(16, 16, "YELLOW", COL_NORMAL);
    draw_tile(1, 17, 'G', 'V', COL_GREEN);
    scr_puts(6, 17, "GREEN", COL_NORMAL);
    draw_tile(11, 17, 'B', 'D', COL_BLUE);
    scr_puts(16, 17, "BLUE", COL_NORMAL);
    draw_tile(1, 18, '?', 'W', COL_WILD);
    scr_puts(6, 18, "WILD", COL_NORMAL);

    scr_puts(1, 20, "S=SKIP V=REVERSE D=DRAW2", COL_NORMAL);
    scr_puts(1, 21, "W=WILD F=WILD+4", COL_NORMAL);
    scr_puts(4, 23, "PRESS SPACE TO START", COL_NORMAL);
}

void ui_draw_frame(void) {
    scr_clear();
    scr_puts(9, TITLE_Y, "*** U N O ***", COL_NORMAL);
}

void ui_draw_opponents(GameState *g) {
    unsigned char opp, x;
    scr_fill_rect(0, OPP_Y, COLS, 1);
    for (opp = 1; opp <= 3; opp++) {
        x = (unsigned char)(1 + (opp - 1) * 10);
        scr_puts(x, OPP_Y, "CPU", COL_NORMAL);
        scr_put((unsigned char)(x + 3), OPP_Y, (char)('0' + opp), COL_NORMAL);
        scr_put((unsigned char)(x + 4), OPP_Y, ':', COL_NORMAL);
        scr_put_num((unsigned char)(x + 5), OPP_Y, g->players[opp].count);
        scr_put((unsigned char)(x + 8), OPP_Y,
                g->current_player == opp ? '<' : ' ', COL_NORMAL);
    }
}

void ui_draw_table(GameState *g) {
    static const char *names[4] = {"RED", "YELLOW", "GREEN", "BLUE"};
    unsigned char col = card_color(g->top_card.color, g->top_color);
    scr_fill_rect(0, TABLE_Y, COLS, 2);

    scr_puts(1, TABLE_Y, "DRAW PILE:", COL_NORMAL);
    scr_put_num(12, TABLE_Y, g->draw_count);
    scr_puts(18, TABLE_Y, "TOP:", COL_NORMAL);
    draw_tile(23, TABLE_Y,
              color_letter(g->top_card.color, g->top_color),
              value_char(g->top_card), col);

    scr_puts(1, TABLE_Y + 1, "COLOR:", COL_NORMAL);
    /* A tile in the live colour, then the name in plain white -- the coloured
       ranges have no E/L/O/N/U to spell the names in their own colour. */
    draw_tile(8, TABLE_Y + 1, color_letter(g->top_color, NONE), ' ', col);
    scr_puts(13, TABLE_Y + 1, names[g->top_color], COL_NORMAL);
    scr_puts(24, TABLE_Y + 1, g->direction > 0 ? "->" : "<-", COL_NORMAL);
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
        x = (unsigned char)(1 + (i % CARDS_PER_ROW) * CARD_W);
        y = (unsigned char)(HAND_Y + i / CARDS_PER_ROW);
        scr_put(x, y, label_char(i), COL_NORMAL);
        draw_tile((unsigned char)(x + 1), y,
                  color_letter(p->hand[i].color, NONE),
                  value_char(p->hand[i]), col);
    }
}

void ui_message(const char *line1, const char *line2) {
    scr_fill_rect(0, MSG_Y1, COLS, 2);
    if (line1) scr_puts(1, MSG_Y1, line1, COL_NORMAL);
    if (line2) scr_puts(1, MSG_Y2, line2, COL_NORMAL);
}

/* Four colour tiles rather than colour names: the selected one switches to
   the highlight range, which is the same cue the hand cursor uses. */
void ui_draw_color_picker(unsigned char selected) {
    static const char letters[4] = {'R', 'Y', 'G', 'B'};
    static const unsigned char cols[4] = {COL_RED, COL_YELLOW, COL_GREEN, COL_BLUE};
    unsigned char i, x;

    scr_fill_rect(0, MSG_Y1, COLS, 2);
    scr_puts(1, MSG_Y1, "CHOOSE A COLOR:", COL_NORMAL);
    for (i = 0; i < 4; i++) {
        x = (unsigned char)(2 + i * 7);
        draw_tile(x, MSG_Y2, letters[i], ' ',
                  (selected == i) ? COL_SELECTED : cols[i]);
    }
}

void ui_clear_color_picker(void) {
    scr_fill_rect(0, MSG_Y1, COLS, 2);
}

static unsigned char player_label(unsigned char x, unsigned char row, unsigned char idx) {
    if (idx == 0) {
        scr_puts(x, row, "YOU", COL_NORMAL);
        return (unsigned char)(x + 4);
    }
    scr_puts(x, row, "CPU", COL_NORMAL);
    scr_put((unsigned char)(x + 3), row, (char)('0' + idx), COL_NORMAL);
    return (unsigned char)(x + 5);
}

void ui_event_skip(unsigned char idx) {
    unsigned char x;
    scr_fill_rect(0, MSG_Y1, COLS, 2);
    x = player_label(1, MSG_Y1, idx);
    scr_puts(x, MSG_Y1, idx == 0 ? "LOSE A TURN" : "IS SKIPPED", COL_NORMAL);
}

void ui_event_reverse(unsigned char idx) {
    (void)idx;
    scr_fill_rect(0, MSG_Y1, COLS, 2);
    scr_puts(1, MSG_Y1, "REVERSE! ORDER FLIPPED", COL_NORMAL);
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
    scr_puts(x, MSG_Y2, "UNO! ONE CARD LEFT!", COL_NORMAL);
}

void ui_event_invalid(void) {
    scr_fill_rect(0, MSG_Y2, COLS, 1);
    scr_puts(1, MSG_Y2, "THAT CARD DOES NOT MATCH!", COL_NORMAL);
}

void ui_event_drew_one(unsigned char idx) {
    unsigned char x;
    scr_fill_rect(0, MSG_Y1, COLS, 2);
    x = player_label(1, MSG_Y1, idx);
    scr_puts(x, MSG_Y1, "NO LEGAL CARD, DREW", COL_NORMAL);
}

void ui_event_thinking(unsigned char idx) {
    unsigned char x;
    scr_fill_rect(0, MSG_Y1, COLS, 2);
    x = player_label(1, MSG_Y1, idx);
    scr_puts(x, MSG_Y1, "IS THINKING...", COL_NORMAL);
}

void ui_draw_challenge_prompt(unsigned char victim, unsigned char player,
                              unsigned char selected_yes) {
    unsigned char x;
    scr_fill_rect(0, MSG_Y1, COLS, 2);
    x = player_label(1, MSG_Y1, player);
    scr_puts(x, MSG_Y1, "PLAYED WILD DRAW 4", COL_NORMAL);
    x = player_label(1, MSG_Y2, victim);
    scr_puts(x, MSG_Y2, "CHALLENGE?", COL_NORMAL);
    x = (unsigned char)(x + 11);
    scr_put(x, MSG_Y2, selected_yes ? '[' : ' ', COL_NORMAL);
    scr_puts((unsigned char)(x + 1), MSG_Y2, "YES", COL_NORMAL);
    scr_put((unsigned char)(x + 4), MSG_Y2, selected_yes ? ']' : ' ', COL_NORMAL);
    scr_put((unsigned char)(x + 6), MSG_Y2, !selected_yes ? '[' : ' ', COL_NORMAL);
    scr_puts((unsigned char)(x + 7), MSG_Y2, "NO", COL_NORMAL);
    scr_put((unsigned char)(x + 9), MSG_Y2, !selected_yes ? ']' : ' ', COL_NORMAL);
}

void ui_event_challenge_result(unsigned char victim, unsigned char player,
                               unsigned char succeeded) {
    unsigned char x;
    scr_fill_rect(0, MSG_Y1, COLS, 2);
    x = player_label(1, MSG_Y1, victim);
    scr_puts(x, MSG_Y1, "CHALLENGES!", COL_NORMAL);
    if (succeeded) {
        x = player_label(1, MSG_Y2, player);
        scr_puts(x, MSG_Y2, "HAD A MATCH - DRAWS 4", COL_NORMAL);
    } else {
        x = player_label(1, MSG_Y2, victim);
        scr_puts(x, MSG_Y2, "WAS WRONG - DRAWS 6", COL_NORMAL);
    }
}

void ui_game_over_screen(unsigned char human_won, unsigned char winner_idx) {
    scr_clear();
    if (human_won) {
        scr_puts(12, 8, "YOU WIN!", COL_NORMAL);
        scr_puts(3, 10, "GREAT GAME, UNO CHAMPION.", COL_NORMAL);
    } else {
        scr_puts(11, 8, "GAME OVER", COL_NORMAL);
        scr_puts(12, 10, "CPU", COL_NORMAL);
        scr_put(15, 10, (char)('0' + winner_idx), COL_NORMAL);
        scr_puts(17, 10, "WINS", COL_NORMAL);
    }
    scr_puts(4, 20, "PRESS SPACE TO PLAY AGAIN", COL_NORMAL);
}
