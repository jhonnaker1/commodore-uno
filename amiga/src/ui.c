#include "ui.h"
#include "amigacon.h"

/* console.device's internal model is 80 columns wide, but only ~65 of them
   are actually visible on this screen (the window is wider than the display
   can show -- see the long comment in amigacon.c). Text centered on the full
   80 therefore lands right of the visible centre, which is what made the
   title look off. Centre banners on the visible width instead. The opponents
   row already sits near this centre, so everything lines up once the title
   and table use it too. */
#define VIS_W 68

static unsigned int ui_strlen(const char *s) {
    unsigned int n = 0;
    while (s[n]) n++;
    return n;
}

static void scr_puts_center(unsigned char y, const char *s, unsigned char color) {
    unsigned int len = ui_strlen(s);
    unsigned char x = len >= VIS_W ? 0 : (unsigned char)((VIS_W - len) / 2);
    scr_puts(x, y, s, color);
}

/* Layout matches the C128 VDC port's 80x25 design (wider cards, spread/
   centered across the full width) since the Amiga's CON: window gives a
   comparable real per-cell-color 80-column text grid -- just via ANSI
   escape codes instead of VDC registers. No custom character set is
   available here (this goes through a console handle, not video RAM we
   can point at our own font), so card borders use plain ASCII (+/-/|)
   instead of custom box-drawing glyphs. */
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

/* The DRAW pile and TOP CARD centre on the visible width (VIS_W), matching
   the title and opponents row above, rather than spreading across the full
   nominal 80 columns (which pushed them right of centre). */
#define DRAW_X 12
#define TOP_X 44

/* The screen's actually-visible width turned out to be narrower than the
   80 columns console.device's internal model claims (confirmed
   empirically -- see the long comment in amigacon.c), so the hand wraps
   to a second row after 9 cards instead of 11, and centers within a
   conservative safe width rather than the full nominal 80, to keep every
   card fully on screen. */
#define HAND_STRIDE 6
#define HAND_PER_ROW 9
/* Centre the hand on the same visible width as the title/table so it lines
   up with them; a full 9-card row is 54 wide, so it still fits comfortably. */
#define HAND_SAFE_WIDTH VIS_W

static const unsigned char suit_color[4] = {COL_RED, COL_YELLOW, COL_GREEN, COL_BLUE};

static void draw_card_box(unsigned char x, unsigned char y, Card c, unsigned char face_up,
                           unsigned char color_override) {
    unsigned char border, glyph;

    if (!face_up) {
        border = COL_WHITE;
        scr_put(x, y, '+', border);
        scr_put(x + 1, y, '-', border);
        scr_put(x + 2, y, '-', border);
        scr_put(x + 3, y, '-', border);
        scr_put(x + 4, y, '+', border);
        scr_put(x, y + 1, '|', border);
        scr_put(x + 1, y + 1, '#', COL_CYAN);
        scr_put(x + 2, y + 1, '#', COL_CYAN);
        scr_put(x + 3, y + 1, '#', COL_CYAN);
        scr_put(x + 4, y + 1, '|', border);
        scr_put(x, y + 2, '|', border);
        scr_put(x + 1, y + 2, '#', COL_CYAN);
        scr_put(x + 2, y + 2, '#', COL_CYAN);
        scr_put(x + 3, y + 2, '#', COL_CYAN);
        scr_put(x + 4, y + 2, '|', border);
        scr_put(x, y + 3, '+', border);
        scr_put(x + 1, y + 3, '-', border);
        scr_put(x + 2, y + 3, '-', border);
        scr_put(x + 3, y + 3, '-', border);
        scr_put(x + 4, y + 3, '+', border);
        return;
    }

    if (c.color != COLOR_WILD) {
        border = suit_color[c.color];
    } else if (color_override != NONE) {
        border = suit_color[color_override];
    } else {
        border = COL_WHITE;
    }

    if (c.value <= 9) {
        glyph = (unsigned char)('0' + c.value);
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

    scr_put(x, y, '+', border);
    scr_put(x + 1, y, '-', border);
    scr_put(x + 2, y, '-', border);
    scr_put(x + 3, y, '-', border);
    scr_put(x + 4, y, '+', border);
    scr_put(x, y + 1, '|', border);
    scr_put(x + 1, y + 1, ' ', COL_WHITE);
    scr_put(x + 2, y + 1, glyph, border);
    scr_put(x + 3, y + 1, ' ', COL_WHITE);
    scr_put(x + 4, y + 1, '|', border);
    scr_put(x, y + 2, '|', border);
    scr_put(x + 1, y + 2, ' ', COL_WHITE);
    scr_put(x + 2, y + 2, ' ', COL_WHITE);
    scr_put(x + 3, y + 2, ' ', COL_WHITE);
    scr_put(x + 4, y + 2, '|', border);
    scr_put(x, y + 3, '+', border);
    scr_put(x + 1, y + 3, '-', border);
    scr_put(x + 2, y + 3, '-', border);
    scr_put(x + 3, y + 3, '-', border);
    scr_put(x + 4, y + 3, '+', border);
}

void ui_title_screen(void) {
    scr_clear();
    scr_puts_center(3, "U    N    O", COL_YELLOW);
    scr_puts_center(5, "FOR THE COMMODORE AMIGA", COL_WHITE);
    scr_puts_center(10, "1 PLAYER VS 3 COMPUTER PLAYERS", COL_LTGRAY);
    scr_puts_center(12, ", AND .: PICK A CARD", COL_CYAN);
    scr_puts_center(13, "SPACE OR RETURN: PLAY / CONFIRM", COL_CYAN);
    scr_puts_center(14, "U: DRAW A CARD", COL_CYAN);
    scr_puts_center(15, "OR PRESS 1-9,0,A-J: PLAY THAT CARD", COL_CYAN);
    scr_puts_center(17, "S=SKIP R=REVERSE D=DRAW2", COL_LTGRAY);
    scr_puts_center(18, "W=WILD F=WILD DRAW FOUR", COL_LTGRAY);

    scr_puts_center(20, "PRESS SPACE TO START", COL_GREEN);
}

void ui_draw_frame(void) {
    scr_clear();
    scr_puts_center(TITLE_Y, "*** U N O ***", COL_YELLOW);
}

void ui_draw_opponents(GameState *g) {
    unsigned char opp, x;
    scr_fill_rect(0, OPP_Y, 80, 1, ' ', COL_BLACK);
    for (opp = 1; opp <= 3; opp++) {
        x = 4 + (opp - 1) * 26;
        scr_put(x, OPP_Y, 'C', COL_WHITE);
        scr_put(x + 1, OPP_Y, 'P', COL_WHITE);
        scr_put(x + 2, OPP_Y, 'U', COL_WHITE);
        scr_put(x + 3, OPP_Y, (char)('0' + opp), COL_WHITE);
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

    scr_fill_rect(0, TABLE_INFO_Y, 80, 1, ' ', COL_BLACK);
    scr_puts(DRAW_X, TABLE_INFO_Y, "x", COL_LTGRAY);
    scr_put_num(DRAW_X + 1, TABLE_INFO_Y, g->draw_count, COL_LTGRAY);

    scr_puts(TOP_X, TABLE_INFO_Y, "COL:", COL_WHITE);
    scr_put(TOP_X + 5, TABLE_INFO_Y, '#', suit_color[g->top_color]);

    scr_puts(TOP_X + 7, TABLE_INFO_Y, g->direction > 0 ? "DIR ->" : "DIR <-", COL_WHITE);
}

/* '1'-'9', '0', then 'A'-'J' for slots 0-19 (matches the quick-play keys). */
static char label_char(unsigned char idx) {
    if (idx < 9) return (char)('1' + idx);
    if (idx == 9) return '0';
    return (char)('A' + (idx - 10));
}

static unsigned char prev_had_row2 = 0;

void ui_draw_hand(GameState *g, unsigned char cursor) {
    Player *p = &g->players[0];
    unsigned char i, x, y, label_y, shown, row_count, start_x;
    unsigned char has_row2 = (p->count > HAND_PER_ROW);

    scr_fill_rect(0, HAND_LABEL_Y, 80, 1, ' ', COL_BLACK);
    scr_puts(1, HAND_LABEL_Y, "YOUR HAND", COL_WHITE);
    scr_put_num(11, HAND_LABEL_Y, p->count, COL_WHITE);

    scr_fill_rect(0, HAND_Y, 80, CARD_H, ' ', COL_BLACK);
    scr_fill_rect(0, LABEL1_Y, 80, 1, ' ', COL_BLACK);
    if (has_row2 || prev_had_row2) {
        scr_fill_rect(0, HAND2_Y, 80, CARD_H, ' ', COL_BLACK);
        scr_fill_rect(0, LABEL2_Y, 80, 1, ' ', COL_BLACK);
    }
    prev_had_row2 = has_row2;

    shown = (p->count > HAND_VISIBLE) ? HAND_VISIBLE : p->count;

    row_count = (shown > HAND_PER_ROW) ? HAND_PER_ROW : shown;
    start_x = (unsigned char)((HAND_SAFE_WIDTH - row_count * HAND_STRIDE) / 2);

    for (i = 0; i < shown; i++) {
        if (i == HAND_PER_ROW) {
            row_count = shown - HAND_PER_ROW;
            start_x = (unsigned char)((HAND_SAFE_WIDTH - row_count * HAND_STRIDE) / 2);
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
    scr_fill_rect(0, MSG_Y1, 80, 2, ' ', COL_BLACK);
    if (line1) scr_puts(1, MSG_Y1, line1, COL_WHITE);
    if (line2) scr_puts(1, MSG_Y2, line2, COL_WHITE);
}

void ui_draw_color_picker(unsigned char selected) {
    static const char *names[4] = {"RED", "YELLOW", "GREEN", "BLUE"};
    unsigned char i, x;

    scr_fill_rect(0, MSG_Y1, 80, 2, ' ', COL_BLACK);
    scr_puts(1, MSG_Y1, "CHOOSE A COLOR:", COL_WHITE);
    for (i = 0; i < 4; i++) {
        x = 10 + i * 16;
        scr_put(x, MSG_Y2, selected == i ? '[' : ' ', COL_WHITE);
        scr_put(x + 1, MSG_Y2, '#', suit_color[i]);
        scr_puts(x + 2, MSG_Y2, names[i], suit_color[i]);
        scr_put(x + 2 + 6, MSG_Y2, selected == i ? ']' : ' ', COL_WHITE);
    }
}

void ui_clear_color_picker(void) {
    scr_fill_rect(0, MSG_Y1, 80, 2, ' ', COL_BLACK);
}

static unsigned char player_label(unsigned char x, unsigned char row, unsigned char idx) {
    if (idx == 0) {
        scr_puts(x, row, "YOU", COL_YELLOW);
        return x + 4;
    }
    scr_puts(x, row, "CPU", COL_YELLOW);
    scr_put(x + 3, row, (char)('0' + idx), COL_YELLOW);
    return x + 5;
}

void ui_event_skip(unsigned char idx) {
    unsigned char x;
    scr_fill_rect(0, MSG_Y1, 80, 2, ' ', COL_BLACK);
    x = player_label(1, MSG_Y1, idx);
    scr_puts(x, MSG_Y1, idx == 0 ? "LOSE A TURN (SKIPPED)" : "IS SKIPPED", COL_WHITE);
}

void ui_event_reverse(unsigned char idx) {
    (void)idx;
    scr_fill_rect(0, MSG_Y1, 80, 2, ' ', COL_BLACK);
    scr_puts(1, MSG_Y1, "REVERSE! PLAY ORDER FLIPPED", COL_WHITE);
}

void ui_event_draw(unsigned char idx, unsigned char count) {
    unsigned char x;
    scr_fill_rect(0, MSG_Y1, 80, 2, ' ', COL_BLACK);
    x = player_label(1, MSG_Y1, idx);
    scr_puts(x, MSG_Y1, idx == 0 ? "MUST DRAW" : "DRAWS", COL_WHITE);
    scr_put_num(x + (idx == 0 ? 10 : 6), MSG_Y1, count, COL_WHITE);
}

void ui_event_uno(unsigned char idx) {
    unsigned char x;
    scr_fill_rect(0, MSG_Y2, 80, 1, ' ', COL_BLACK);
    x = player_label(1, MSG_Y2, idx);
    scr_puts(x, MSG_Y2, "UNO! ONE CARD LEFT!", COL_YELLOW);
}

void ui_event_invalid(void) {
    scr_fill_rect(0, MSG_Y2, 80, 1, ' ', COL_BLACK);
    scr_puts(1, MSG_Y2, "THAT CARD DOES NOT MATCH!", COL_RED);
}

void ui_event_drew_one(unsigned char idx) {
    unsigned char x;
    scr_fill_rect(0, MSG_Y1, 80, 2, ' ', COL_BLACK);
    x = player_label(1, MSG_Y1, idx);
    scr_puts(x, MSG_Y1, "NO LEGAL CARD, DREW ONE", COL_WHITE);
}

void ui_event_thinking(unsigned char idx) {
    unsigned char x;
    scr_fill_rect(0, MSG_Y1, 80, 2, ' ', COL_BLACK);
    x = player_label(1, MSG_Y1, idx);
    scr_puts(x, MSG_Y1, "IS THINKING...", COL_LTGRAY);
}

void ui_draw_challenge_prompt(unsigned char victim, unsigned char player, unsigned char selected_yes) {
    unsigned char x;
    scr_fill_rect(0, MSG_Y1, 80, 2, ' ', COL_BLACK);
    x = player_label(1, MSG_Y1, player);
    scr_puts(x, MSG_Y1, "PLAYED WILD DRAW FOUR", COL_WHITE);
    x = player_label(1, MSG_Y2, victim);
    scr_puts(x, MSG_Y2, "CHALLENGE?", COL_WHITE);
    x += 11;
    scr_put(x, MSG_Y2, selected_yes ? '[' : ' ', COL_WHITE);
    scr_puts(x + 1, MSG_Y2, "YES", selected_yes ? COL_GREEN : COL_LTGRAY);
    scr_put(x + 4, MSG_Y2, selected_yes ? ']' : ' ', COL_WHITE);
    scr_put(x + 6, MSG_Y2, !selected_yes ? '[' : ' ', COL_WHITE);
    scr_puts(x + 7, MSG_Y2, "NO", !selected_yes ? COL_RED : COL_LTGRAY);
    scr_put(x + 9, MSG_Y2, !selected_yes ? ']' : ' ', COL_WHITE);
}

void ui_event_challenge_result(unsigned char victim, unsigned char player, unsigned char succeeded) {
    unsigned char x;
    scr_fill_rect(0, MSG_Y1, 80, 2, ' ', COL_BLACK);
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
        scr_puts_center(8, "YOU WIN!", COL_GREEN);
        scr_puts_center(10, "GREAT GAME, UNO CHAMPION.", COL_WHITE);
    } else {
        unsigned char wx = (VIS_W - 9) / 2;   /* centre "CPUn WINS" (9 chars) */
        scr_puts_center(8, "GAME OVER", COL_RED);
        scr_put(wx, 10, 'C', COL_WHITE);
        scr_put(wx + 1, 10, 'P', COL_WHITE);
        scr_put(wx + 2, 10, 'U', COL_WHITE);
        scr_put(wx + 3, 10, (char)('0' + winner_idx), COL_WHITE);
        scr_puts(wx + 5, 10, "WINS", COL_WHITE);
    }
    scr_puts_center(20, "PRESS SPACE TO PLAY AGAIN", COL_CYAN);
}
