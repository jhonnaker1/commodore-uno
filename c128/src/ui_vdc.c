#include <ascii_charmap.h>
#include "ui.h"
#include "vdc.h"
#include "charset_codes.h"

/* Layout tuned for the VDC's real 80x25 text grid -- distinct from the
   40-column ui.c this was cloned from. Card boxes are wider (5 cols
   instead of 3) and everything is spread/centered across the full 80
   columns instead of packed into the left 40, since 80-column character
   cells are physically narrower than 40-column ones (same overall screen
   width, twice the columns), so using more columns per element is what
   keeps things looking reasonably sized rather than tiny and bunched up. */
#define TITLE_Y 0
#define OPP_Y 2
#define TABLE_LABEL_Y 4
#define CARD_Y 5
#define TABLE_INFO_Y 9
#define MSG_Y1 11
#define MSG_Y2 12
#define HAND_LABEL_Y 14
#define HAND_Y 15
#define LABEL1_Y 19
#define HAND2_Y 20
#define LABEL2_Y 24

#define CARD_W 5
#define CARD_H 4

#define DRAW_X 18
#define TOP_X 50

/* Hand cards are 5 wide with a 2-col gap (stride 7); 11 fit per row within
   80 columns (11*7=77), so a full 20-card hand (HAND_VISIBLE) splits into
   an 11-card row and a 9-card row rather than the 10/10 split the
   40-column version uses. */
#define HAND_STRIDE 7
#define HAND_PER_ROW 11

static const unsigned char suit_color[4] = {COL_RED, COL_YELLOW, COL_GREEN, COL_BLUE};

static void draw_card_box(unsigned char x, unsigned char y, Card c, unsigned char face_up,
                           unsigned char color_override) {
    unsigned char border, glyph;

    if (!face_up) {
        border = COL_LTGRAY;
        scr_put(x, y, CH_CORNER_TL, border);
        scr_put(x + 1, y, CH_EDGE_TOP, border);
        scr_put(x + 2, y, CH_EDGE_TOP, border);
        scr_put(x + 3, y, CH_EDGE_TOP, border);
        scr_put(x + 4, y, CH_CORNER_TR, border);
        scr_put(x, y + 1, CH_EDGE_LEFT, border);
        scr_put(x + 1, y + 1, CH_CARDBACK, COL_CYAN);
        scr_put(x + 2, y + 1, CH_CARDBACK, COL_CYAN);
        scr_put(x + 3, y + 1, CH_CARDBACK, COL_CYAN);
        scr_put(x + 4, y + 1, CH_EDGE_RIGHT, border);
        scr_put(x, y + 2, CH_EDGE_LEFT, border);
        scr_put(x + 1, y + 2, CH_CARDBACK, COL_CYAN);
        scr_put(x + 2, y + 2, CH_CARDBACK, COL_CYAN);
        scr_put(x + 3, y + 2, CH_CARDBACK, COL_CYAN);
        scr_put(x + 4, y + 2, CH_EDGE_RIGHT, border);
        scr_put(x, y + 3, CH_CORNER_BL, border);
        scr_put(x + 1, y + 3, CH_EDGE_BOTTOM, border);
        scr_put(x + 2, y + 3, CH_EDGE_BOTTOM, border);
        scr_put(x + 3, y + 3, CH_EDGE_BOTTOM, border);
        scr_put(x + 4, y + 3, CH_CORNER_BR, border);
        return;
    }

    if (c.color != COLOR_WILD) {
        border = suit_color[c.color];
    } else if (color_override != NONE) {
        border = suit_color[color_override];
    } else {
        border = COL_LTGRAY;
    }

    if (c.value <= 9) {
        glyph = CH_DIGIT_0 + c.value;
    } else if (c.value == VAL_SKIP) {
        glyph = 'S';
    } else if (c.value == VAL_REVERSE) {
        glyph = 'R';
    } else if (c.value == VAL_DRAW2) {
        glyph = 'D';
    } else if (c.value == VAL_WILD) {
        glyph = 'W';
    } else {
        glyph = 'F'; /* wild draw four - distinct from 'W' and from digit 4 */
    }

    scr_put(x, y, CH_CORNER_TL, border);
    scr_put(x + 1, y, CH_EDGE_TOP, border);
    scr_put(x + 2, y, CH_EDGE_TOP, border);
    scr_put(x + 3, y, CH_EDGE_TOP, border);
    scr_put(x + 4, y, CH_CORNER_TR, border);
    scr_put(x, y + 1, CH_EDGE_LEFT, border);
    scr_put(x + 1, y + 1, ' ', COL_WHITE);
    scr_put(x + 2, y + 1, glyph, COL_WHITE);
    scr_put(x + 3, y + 1, ' ', COL_WHITE);
    scr_put(x + 4, y + 1, CH_EDGE_RIGHT, border);
    scr_put(x, y + 2, CH_EDGE_LEFT, border);
    scr_put(x + 1, y + 2, ' ', COL_WHITE);
    scr_put(x + 2, y + 2, ' ', COL_WHITE);
    scr_put(x + 3, y + 2, ' ', COL_WHITE);
    scr_put(x + 4, y + 2, CH_EDGE_RIGHT, border);
    scr_put(x, y + 3, CH_CORNER_BL, border);
    scr_put(x + 1, y + 3, CH_EDGE_BOTTOM, border);
    scr_put(x + 2, y + 3, CH_EDGE_BOTTOM, border);
    scr_put(x + 3, y + 3, CH_EDGE_BOTTOM, border);
    scr_put(x + 4, y + 3, CH_CORNER_BR, border);
}

void ui_title_screen(void) {
    scr_clear();
    scr_puts(34, 3, "U    N    O", COL_YELLOW);
    scr_puts(23, 5, "FOR THE COMMODORE 128 (80-COL VDC)", COL_WHITE);
    scr_puts(25, 10, "1 PLAYER VS 3 COMPUTER PLAYERS", COL_LTGRAY);
    scr_puts(28, 12, "JOYSTICK IN PORT 2, OR:", COL_CYAN);
    scr_puts(30, 13, ", AND .: PICK A CARD", COL_CYAN);
    scr_puts(24, 14, "SPACE OR RETURN: PLAY / CONFIRM", COL_CYAN);
    scr_puts(33, 15, "U: DRAW A CARD", COL_CYAN);
    scr_puts(23, 16, "OR PRESS 1-9,0,A-J: PLAY THAT CARD", COL_CYAN);
    scr_puts(28, 17, "S=SKIP R=REVERSE D=DRAW2", COL_LTGRAY);
    scr_puts(28, 18, "W=WILD F=WILD DRAW FOUR", COL_LTGRAY);

    scr_puts(30, 20, "PRESS FIRE TO START", COL_GREEN);
}

void ui_draw_frame(void) {
    scr_clear();
    scr_puts(33, TITLE_Y, "*** U N O ***", COL_YELLOW);
}

void ui_draw_opponents(GameState *g) {
    unsigned char opp, x;
    scr_fill_rect(0, OPP_Y, 80, 1, 32, COL_BLACK);
    for (opp = 1; opp <= 3; opp++) {
        x = 4 + (opp - 1) * 26;
        scr_put(x, OPP_Y, 'C', COL_WHITE);
        scr_put(x + 1, OPP_Y, 'P', COL_WHITE);
        scr_put(x + 2, OPP_Y, 'U', COL_WHITE);
        scr_put(x + 3, OPP_Y, '0' + opp, COL_WHITE);
        scr_put(x + 4, OPP_Y, ':', COL_WHITE);
        scr_put_num(x + 6, OPP_Y, g->players[opp].count, COL_LTGRAY);
        if (g->current_player == opp) {
            scr_put(x + 9, OPP_Y, '<', COL_YELLOW);
        } else {
            scr_put(x + 9, OPP_Y, ' ', COL_BLACK);
        }
    }
}

void ui_draw_table(GameState *g) {
    Card back;
    back.color = COLOR_WILD;
    back.value = VAL_WILD;

    scr_puts(DRAW_X, TABLE_LABEL_Y, "DRAW", COL_LTGRAY);
    scr_puts(TOP_X, TABLE_LABEL_Y, "TOP CARD", COL_LTGRAY);

    draw_card_box(DRAW_X, CARD_Y, back, 0, NONE);
    draw_card_box(TOP_X, CARD_Y, g->top_card, 1, g->top_color);

    scr_fill_rect(0, TABLE_INFO_Y, 80, 1, 32, COL_BLACK);
    scr_puts(DRAW_X, TABLE_INFO_Y, "x", COL_LTGRAY);
    scr_put_num(DRAW_X + 1, TABLE_INFO_Y, g->draw_count, COL_LTGRAY);

    scr_puts(TOP_X, TABLE_INFO_Y, "COL:", COL_WHITE);
    scr_put(TOP_X + 5, TABLE_INFO_Y, CH_SOLID_BLOCK, suit_color[g->top_color]);

    scr_puts(TOP_X + 7, TABLE_INFO_Y, g->direction > 0 ? "DIR ->" : "DIR <-", COL_WHITE);
}

/* '1'-'9', '0', then 'A'-'J' for slots 0-19 (matches the quick-play keys). */
static char label_char(unsigned char idx) {
    if (idx < 9) return (char)('1' + idx);
    if (idx == 9) return '0';
    return (char)('A' + (idx - 10));
}

/* Whether the previous call drew a second row, so we know whether that
   area needs clearing this time (skipping it when not is a meaningful
   chunk of work off the game's single heaviest screen update). */
static unsigned char prev_had_row2 = 0;

void ui_draw_hand(GameState *g, unsigned char cursor) {
    Player *p = &g->players[0];
    unsigned char i, x, y, label_y, shown, row_count, start_x;
    unsigned char has_row2 = (p->count > HAND_PER_ROW);

    scr_fill_rect(0, HAND_LABEL_Y, 80, 1, 32, COL_BLACK);
    scr_puts(1, HAND_LABEL_Y, "YOUR HAND", COL_WHITE);
    scr_put_num(11, HAND_LABEL_Y, p->count, COL_WHITE);

    scr_fill_rect(0, HAND_Y, 80, CARD_H, 32, COL_BLACK);
    scr_fill_rect(0, LABEL1_Y, 80, 1, 32, COL_BLACK);
    if (has_row2 || prev_had_row2) {
        scr_fill_rect(0, HAND2_Y, 80, CARD_H, 32, COL_BLACK);
        scr_fill_rect(0, LABEL2_Y, 80, 1, 32, COL_BLACK);
    }
    prev_had_row2 = has_row2;

    shown = (p->count > HAND_VISIBLE) ? HAND_VISIBLE : p->count;

    /* Center each row's cards rather than left-packing them, so a typical
       (well under 11-card) hand doesn't look stranded on the left half of
       an 80-column screen. */
    row_count = (shown > HAND_PER_ROW) ? HAND_PER_ROW : shown;
    start_x = (unsigned char)((80 - row_count * HAND_STRIDE) / 2);

    for (i = 0; i < shown; i++) {
        if (i == HAND_PER_ROW) {
            row_count = shown - HAND_PER_ROW;
            start_x = (unsigned char)((80 - row_count * HAND_STRIDE) / 2);
        }
        x = start_x + (i % HAND_PER_ROW) * HAND_STRIDE;
        y = (i < HAND_PER_ROW) ? HAND_Y : HAND2_Y;
        label_y = (i < HAND_PER_ROW) ? LABEL1_Y : LABEL2_Y;
        draw_card_box(x, y, p->hand[i], 1, NONE);
        if (i == cursor) {
            scr_put(x + 1, label_y, '[', COL_YELLOW);
            scr_put(x + 2, label_y, label_char(i), COL_YELLOW);
            scr_put(x + 3, label_y, ']', COL_YELLOW);
        } else {
            scr_put(x + 2, label_y, label_char(i), COL_LTGRAY);
        }
    }
}

void ui_message(const char *line1, const char *line2) {
    scr_fill_rect(0, MSG_Y1, 80, 2, 32, COL_BLACK);
    if (line1) scr_puts(1, MSG_Y1, line1, COL_WHITE);
    if (line2) scr_puts(1, MSG_Y2, line2, COL_WHITE);
}

void ui_draw_color_picker(unsigned char selected) {
    static const char *names[4] = {"RED", "YELLOW", "GREEN", "BLUE"};
    unsigned char i, x;

    scr_fill_rect(0, MSG_Y1, 80, 2, 32, COL_BLACK);
    scr_puts(1, MSG_Y1, "CHOOSE A COLOR:", COL_WHITE);
    for (i = 0; i < 4; i++) {
        x = 10 + i * 16;
        scr_put(x, MSG_Y2, selected == i ? '[' : 32, COL_WHITE);
        scr_put(x + 1, MSG_Y2, CH_SOLID_BLOCK, suit_color[i]);
        scr_puts(x + 2, MSG_Y2, names[i], suit_color[i]);
        scr_put(x + 2 + 6, MSG_Y2, selected == i ? ']' : 32, COL_WHITE);
    }
}

void ui_clear_color_picker(void) {
    scr_fill_rect(0, MSG_Y1, 80, 2, 32, COL_BLACK);
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
    scr_fill_rect(0, MSG_Y1, 80, 2, 32, COL_BLACK);
    x = player_label(1, MSG_Y1, idx);
    scr_puts(x, MSG_Y1, idx == 0 ? "LOSE A TURN (SKIPPED)" : "IS SKIPPED", COL_WHITE);
}

void ui_event_reverse(unsigned char idx) {
    (void)idx;
    scr_fill_rect(0, MSG_Y1, 80, 2, 32, COL_BLACK);
    scr_puts(1, MSG_Y1, "REVERSE! PLAY ORDER FLIPPED", COL_WHITE);
}

void ui_event_draw(unsigned char idx, unsigned char count) {
    unsigned char x;
    scr_fill_rect(0, MSG_Y1, 80, 2, 32, COL_BLACK);
    x = player_label(1, MSG_Y1, idx);
    scr_puts(x, MSG_Y1, idx == 0 ? "MUST DRAW" : "DRAWS", COL_WHITE);
    scr_put_num(x + (idx == 0 ? 10 : 6), MSG_Y1, count, COL_WHITE);
}

void ui_event_uno(unsigned char idx) {
    unsigned char x;
    scr_fill_rect(0, MSG_Y2, 80, 1, 32, COL_BLACK);
    x = player_label(1, MSG_Y2, idx);
    scr_puts(x, MSG_Y2, "UNO! ONE CARD LEFT!", COL_YELLOW);
}

void ui_event_invalid(void) {
    scr_fill_rect(0, MSG_Y2, 80, 1, 32, COL_BLACK);
    scr_puts(1, MSG_Y2, "THAT CARD DOES NOT MATCH!", COL_RED);
}

void ui_event_drew_one(unsigned char idx) {
    unsigned char x;
    scr_fill_rect(0, MSG_Y1, 80, 2, 32, COL_BLACK);
    x = player_label(1, MSG_Y1, idx);
    scr_puts(x, MSG_Y1, "NO LEGAL CARD, DREW ONE", COL_WHITE);
}

void ui_event_thinking(unsigned char idx) {
    unsigned char x;
    scr_fill_rect(0, MSG_Y1, 80, 2, 32, COL_BLACK);
    x = player_label(1, MSG_Y1, idx);
    scr_puts(x, MSG_Y1, "IS THINKING...", COL_LTGRAY);
}

void ui_draw_challenge_prompt(unsigned char victim, unsigned char player, unsigned char selected_yes) {
    unsigned char x;
    scr_fill_rect(0, MSG_Y1, 80, 2, 32, COL_BLACK);
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
    scr_fill_rect(0, MSG_Y1, 80, 2, 32, COL_BLACK);
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
        scr_puts(36, 8, "YOU WIN!", COL_GREEN);
        scr_puts(27, 10, "GREAT GAME, UNO CHAMPION.", COL_WHITE);
    } else {
        scr_puts(35, 8, "GAME OVER", COL_RED);
        scr_put(37, 10, 'C', COL_WHITE);
        scr_put(38, 10, 'P', COL_WHITE);
        scr_put(39, 10, 'U', COL_WHITE);
        scr_put(40, 10, '0' + winner_idx, COL_WHITE);
        scr_puts(42, 10, "WINS", COL_WHITE);
    }
    scr_puts(28, 20, "PRESS FIRE TO PLAY AGAIN", COL_CYAN);
}
