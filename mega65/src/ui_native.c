#include "ui.h"
#include "m65native.h"

/* The 80-column native-mode UI. Same screens as the 40-column ui.c, laid
   out for twice the width -- the proportions come from the C128 port's
   ui_vdc.c, which solved the same problem for the VDC's 80x25.

   Two things differ from that one. It uploads a custom charset to get card
   corners and big digits; this uses the stock PETSCII box graphics, because
   the MEGA65's per-cell colour makes a solid tile carry the card better
   than a drawn outline would. And its colour constants needed a whole
   RGBI translation table, because the VDC is a different chip with a
   different palette; the VIC-IV shares the VIC-II's, so COL_* here are
   simply the real palette indices.

   Cards are 5x4 rather than 3x4. The extra width is what allows a solid
   colour block with the value knocked out of it, instead of a lone glyph
   between two border characters. */
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

/* Hand cards are 5 wide with a 2-column gap (stride 7), so 11 fit across 80
   columns (1 + 10*7 + 5 = 76). A full 20-card hand therefore splits 11/9,
   where the 40-column build splits 10/10. */
#define HAND_STRIDE 7
#define HAND_PER_ROW 11

static const unsigned char suit_color[4] = {COL_RED, COL_YELLOW, COL_GREEN, COL_BLUE};

static void draw_card_box(unsigned char x, unsigned char y, Card c, unsigned char face_up,
                           unsigned char color_override) {
    unsigned char border, glyph, fill, i;

    if (face_up) {
        if (c.color != COLOR_WILD) {
            border = suit_color[c.color];
        } else if (color_override != NONE) {
            border = suit_color[color_override];
        } else {
            border = COL_LTGRAY;
        }
        fill = border;
    } else {
        border = COL_LTGRAY;
        fill = COL_CYAN;
    }

    /* Border: corners and edges, in the suit colour. */
    scr_put(x, y, CH_ULCORNER, border);
    scr_put(x + CARD_W - 1, y, CH_URCORNER, border);
    scr_put(x, y + CARD_H - 1, CH_LLCORNER, border);
    scr_put(x + CARD_W - 1, y + CARD_H - 1, CH_LRCORNER, border);
    for (i = 1; i < CARD_W - 1; i++) {
        scr_put(x + i, y, CH_HLINE, border);
        scr_put(x + i, y + CARD_H - 1, CH_HLINE, border);
    }
    scr_put(x, y + 1, CH_VLINE, border);
    scr_put(x, y + 2, CH_VLINE, border);
    scr_put(x + CARD_W - 1, y + 1, CH_VLINE, border);
    scr_put(x + CARD_W - 1, y + 2, CH_VLINE, border);

    /* Interior: a solid block of the suit colour. */
    for (i = 1; i < CARD_W - 1; i++) {
        scr_put_solid(x + i, y + 1, fill);
        scr_put_solid(x + i, y + 2, fill);
    }

    if (!face_up) return;

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

    /* The value, knocked out of the solid block by the reverse attribute. */
    scr_put(x + 2, y + 1, glyph, (unsigned char)(fill | COL_REVERSE));
}

void ui_title_screen(void) {
    scr_clear();
    scr_puts(37, 3, "U N O", COL_YELLOW);
    scr_puts(27, 5, "FOR THE MEGA65 IN NATIVE MODE", COL_WHITE);
    scr_puts(30, 6, "80 COLUMNS ON THE VIC-IV", COL_MDGRAY);

    scr_puts(25, 9, "1 PLAYER VS 3 COMPUTER PLAYERS", COL_LTGRAY);

    scr_puts(24, 12, "JOYSTICK IN PORT 2, OR:", COL_CYAN);
    scr_puts(24, 13, "CURSOR LEFT/RIGHT (OR , AND .): PICK A CARD", COL_CYAN);
    scr_puts(24, 14, "SPACE OR RETURN: PLAY / CONFIRM", COL_CYAN);
    scr_puts(24, 15, "U OR CURSOR UP: DRAW A CARD", COL_CYAN);
    scr_puts(24, 16, "OR PRESS 1-9, 0, A-J: PLAY THAT CARD", COL_CYAN);

    scr_puts(24, 18, "S=SKIP  R=REVERSE  D=DRAW TWO", COL_LTGRAY);
    scr_puts(24, 19, "W=WILD  F=WILD DRAW FOUR", COL_LTGRAY);

    scr_puts(30, 22, "PRESS FIRE TO START", COL_GREEN);
}

void ui_draw_frame(void) {
    scr_clear();
    scr_puts(33, TITLE_Y, "*** U N O ***", COL_YELLOW);
}

void ui_draw_opponents(GameState *g) {
    unsigned char opp, x;
    scr_fill_rect(0, OPP_Y, COLS, 1, ' ', COL_BLACK);
    for (opp = 1; opp <= 3; opp++) {
        /* Three evenly spaced slots across 80 columns. The 40-column build
           has to abbreviate to "CPU1:7"; there is room here for words. */
        x = 4 + (opp - 1) * 25;
        scr_puts(x, OPP_Y, "CPU", COL_WHITE);
        scr_put(x + 3, OPP_Y, (unsigned char)('0' + opp), COL_WHITE);
        scr_puts(x + 5, OPP_Y, "CARDS:", COL_LTGRAY);
        scr_put_num(x + 12, OPP_Y, g->players[opp].count, COL_WHITE);
        if (g->current_player == opp) {
            scr_puts(x + 15, OPP_Y, "<< TURN", COL_YELLOW);
        } else {
            scr_fill_rect(x + 15, OPP_Y, 7, 1, ' ', COL_BLACK);
        }
    }
}

void ui_draw_table(GameState *g) {
    Card back;
    back.color = COLOR_WILD;
    back.value = VAL_WILD;

    scr_puts(DRAW_X, TABLE_LABEL_Y, "DRAW PILE", COL_LTGRAY);
    scr_puts(TOP_X - 2, TABLE_LABEL_Y, "TOP CARD", COL_LTGRAY);

    draw_card_box(DRAW_X, CARD_Y, back, 0, NONE);
    draw_card_box(TOP_X, CARD_Y, g->top_card, 1, g->top_color);

    scr_fill_rect(0, TABLE_INFO_Y, COLS, 1, ' ', COL_BLACK);
    /* Uppercase only, everywhere in this file. setuppercase() selects the
       upper/graphics charset, where screen codes $41-$5A are graphics
       symbols rather than lowercase letters -- so a lowercase letter in a
       string comes out as a piece of box-drawing. The 40-column build
       carries the same constraint. There is room at 80 columns to spell
       this out rather than abbreviate it to "x79". */
    scr_puts(DRAW_X, TABLE_INFO_Y, "CARDS:", COL_LTGRAY);
    scr_put_num(DRAW_X + 7, TABLE_INFO_Y, g->draw_count, COL_LTGRAY);

    scr_puts(TOP_X - 2, TABLE_INFO_Y, "COLOR:", COL_WHITE);
    scr_put_solid(TOP_X + 5, TABLE_INFO_Y, suit_color[g->top_color]);
    scr_put_solid(TOP_X + 6, TABLE_INFO_Y, suit_color[g->top_color]);

    scr_puts(TOP_X + 9, TABLE_INFO_Y,
             g->direction > 0 ? "DIRECTION ->" : "DIRECTION <-", COL_WHITE);
}

/* '1'-'9', '0', then 'A'-'J' for slots 0-19 (matches the quick-play keys). */
static char label_char(unsigned char idx) {
    if (idx < 9) return (char)('1' + idx);
    if (idx == 9) return '0';
    return (char)('A' + (idx - 10));
}

/* Whether the previous call drew a second row, so we know whether that
   area needs clearing this time. */
static unsigned char prev_had_row2 = 0;

void ui_draw_hand(GameState *g, unsigned char cursor) {
    Player *p = &g->players[0];
    unsigned char i, x, y, label_y, shown;
    unsigned char has_row2 = (p->count > HAND_PER_ROW);

    scr_fill_rect(0, HAND_LABEL_Y, COLS, 1, ' ', COL_BLACK);
    scr_puts(1, HAND_LABEL_Y, "YOUR HAND:", COL_WHITE);
    scr_put_num(12, HAND_LABEL_Y, p->count, COL_WHITE);
    scr_puts(16, HAND_LABEL_Y, "CARDS", COL_LTGRAY);

    scr_fill_rect(0, HAND_Y, COLS, CARD_H, ' ', COL_BLACK);
    scr_fill_rect(0, LABEL1_Y, COLS, 1, ' ', COL_BLACK);
    if (has_row2 || prev_had_row2) {
        scr_fill_rect(0, HAND2_Y, COLS, CARD_H, ' ', COL_BLACK);
        scr_fill_rect(0, LABEL2_Y, COLS, 1, ' ', COL_BLACK);
    }
    prev_had_row2 = has_row2;

    shown = (p->count > HAND_VISIBLE) ? HAND_VISIBLE : p->count;

    for (i = 0; i < shown; i++) {
        x = (unsigned char)(1 + (i % HAND_PER_ROW) * HAND_STRIDE);
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
    scr_fill_rect(0, MSG_Y1, COLS, 2, ' ', COL_BLACK);
    if (line1) scr_puts(2, MSG_Y1, line1, COL_WHITE);
    if (line2) scr_puts(2, MSG_Y2, line2, COL_WHITE);
}

void ui_draw_color_picker(unsigned char selected) {
    static const char *names[4] = {"RED", "YELLOW", "GREEN", "BLUE"};
    unsigned char i, x;

    scr_fill_rect(0, MSG_Y1, COLS, 2, ' ', COL_BLACK);
    scr_puts(2, MSG_Y1, "CHOOSE A COLOR:", COL_WHITE);
    for (i = 0; i < 4; i++) {
        x = (unsigned char)(4 + i * 17);
        scr_put(x, MSG_Y2, selected == i ? '>' : ' ', COL_WHITE);
        scr_put_solid(x + 2, MSG_Y2, suit_color[i]);
        scr_put_solid(x + 3, MSG_Y2, suit_color[i]);
        scr_puts(x + 5, MSG_Y2, names[i], suit_color[i]);
        scr_put(x + 12, MSG_Y2, selected == i ? '<' : ' ', COL_WHITE);
    }
}

void ui_clear_color_picker(void) {
    scr_fill_rect(0, MSG_Y1, COLS, 2, ' ', COL_BLACK);
}

static unsigned char player_label(unsigned char x, unsigned char row, unsigned char idx) {
    if (idx == 0) {
        scr_puts(x, row, "YOU", COL_YELLOW);
        return (unsigned char)(x + 4);
    }
    scr_puts(x, row, "CPU", COL_YELLOW);
    scr_put(x + 3, row, (unsigned char)('0' + idx), COL_YELLOW);
    return (unsigned char)(x + 5);
}

void ui_event_skip(unsigned char idx) {
    unsigned char x;
    scr_fill_rect(0, MSG_Y1, COLS, 2, ' ', COL_BLACK);
    x = player_label(2, MSG_Y1, idx);
    scr_puts(x, MSG_Y1, idx == 0 ? "LOSE A TURN (SKIPPED)" : "IS SKIPPED", COL_WHITE);
}

void ui_event_reverse(unsigned char idx) {
    (void)idx;
    scr_fill_rect(0, MSG_Y1, COLS, 2, ' ', COL_BLACK);
    scr_puts(2, MSG_Y1, "REVERSE! PLAY ORDER FLIPPED", COL_WHITE);
}

void ui_event_draw(unsigned char idx, unsigned char count) {
    unsigned char x;
    scr_fill_rect(0, MSG_Y1, COLS, 2, ' ', COL_BLACK);
    x = player_label(2, MSG_Y1, idx);
    scr_puts(x, MSG_Y1, idx == 0 ? "MUST DRAW" : "DRAWS", COL_WHITE);
    scr_put_num((unsigned char)(x + (idx == 0 ? 10 : 6)), MSG_Y1, count, COL_WHITE);
}

void ui_event_uno(unsigned char idx) {
    unsigned char x;
    scr_fill_rect(0, MSG_Y2, COLS, 1, ' ', COL_BLACK);
    x = player_label(2, MSG_Y2, idx);
    scr_puts(x, MSG_Y2, "UNO! ONE CARD LEFT!", COL_YELLOW);
}

void ui_event_invalid(void) {
    scr_fill_rect(0, MSG_Y2, COLS, 1, ' ', COL_BLACK);
    scr_puts(2, MSG_Y2, "THAT CARD DOES NOT MATCH!", COL_RED);
}

void ui_event_drew_one(unsigned char idx) {
    unsigned char x;
    scr_fill_rect(0, MSG_Y1, COLS, 2, ' ', COL_BLACK);
    x = player_label(2, MSG_Y1, idx);
    scr_puts(x, MSG_Y1, "NO LEGAL CARD, DREW ONE", COL_WHITE);
}

void ui_event_thinking(unsigned char idx) {
    unsigned char x;
    scr_fill_rect(0, MSG_Y1, COLS, 2, ' ', COL_BLACK);
    x = player_label(2, MSG_Y1, idx);
    scr_puts(x, MSG_Y1, "IS THINKING...", COL_LTGRAY);
}

void ui_draw_challenge_prompt(unsigned char victim, unsigned char player, unsigned char selected_yes) {
    unsigned char x;
    scr_fill_rect(0, MSG_Y1, COLS, 2, ' ', COL_BLACK);
    x = player_label(2, MSG_Y1, player);
    scr_puts(x, MSG_Y1, "PLAYED WILD DRAW FOUR", COL_WHITE);
    x = player_label(2, MSG_Y2, victim);
    scr_puts(x, MSG_Y2, "CHALLENGE?", COL_WHITE);
    x = (unsigned char)(x + 12);
    scr_put(x, MSG_Y2, selected_yes ? '>' : ' ', COL_WHITE);
    scr_puts(x + 2, MSG_Y2, "YES", selected_yes ? COL_GREEN : COL_LTGRAY);
    scr_put(x + 6, MSG_Y2, selected_yes ? '<' : ' ', COL_WHITE);
    scr_put(x + 9, MSG_Y2, !selected_yes ? '>' : ' ', COL_WHITE);
    scr_puts(x + 11, MSG_Y2, "NO", !selected_yes ? COL_RED : COL_LTGRAY);
    scr_put(x + 14, MSG_Y2, !selected_yes ? '<' : ' ', COL_WHITE);
}

void ui_event_challenge_result(unsigned char victim, unsigned char player, unsigned char succeeded) {
    unsigned char x;
    scr_fill_rect(0, MSG_Y1, COLS, 2, ' ', COL_BLACK);
    x = player_label(2, MSG_Y1, victim);
    scr_puts(x, MSG_Y1, "CHALLENGES!", COL_YELLOW);
    if (succeeded) {
        x = player_label(2, MSG_Y2, player);
        scr_puts(x, MSG_Y2, "HAD A MATCH - DRAWS 4", COL_GREEN);
    } else {
        x = player_label(2, MSG_Y2, victim);
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
        scr_puts(35, 10, "CPU", COL_WHITE);
        scr_put(38, 10, (unsigned char)('0' + winner_idx), COL_WHITE);
        scr_puts(40, 10, "WINS", COL_WHITE);
    }
    scr_puts(28, 20, "PRESS FIRE TO PLAY AGAIN", COL_CYAN);
}
