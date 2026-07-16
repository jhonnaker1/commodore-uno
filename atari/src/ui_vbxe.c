#include "ui.h"
#include "ui_vbxe.h"
#include "vbxevid.h"

/* VBXE's text overlay mode gives real per-character color (128
   foreground colors from a 1024-color palette -- see vbxevid.c), so
   unlike atari/src/ui.c (stock Atari text mode, one fg/bg for the
   whole screen), cards render as solid colored TILES -- a real card
   shape, not a color-letter bracket -- matching the level of the C64
   port's redefined-charset card graphics. Since VBXE's per-cell
   attribute ties a foreground index to its own fixed background
   (index+128, see make_attr() in vbxevid.c), each suit gets its own
   dedicated TILE_* palette index carrying a colored background with a
   contrasting foreground for the value character drawn on top (see
   vbxevid.c's palette table).

   VBXE has no hardware sprites (unlike the C64/C128 ports this mirrors
   the feature set of), so the "toss" animation moves a small colored
   block cell-by-cell across the text grid instead of a real sprite --
   see animate_toss_to() in main_vbxe.c. The selection cursor is a pair
   of bracket cells flanking the tile, toggled independently of the
   tile itself (ui_blink_cursor()), the same trick the C64 port uses
   for its own bracket toggle. */

#define TITLE_Y 0
#define OPP_Y 2
#define TABLE_Y 4
#define TABLE_TILE_X 60
#define MSG_Y1 8
#define MSG_Y2 9
#define HAND_LABEL_Y 11
#define HAND_Y 12
#define CARDS_PER_ROW 10
#define SLOT_W 5 /* 1 bracket + 3 tile + 1 bracket */
#define SLOT_H 4 /* 3 tile rows + 1 label row */

static unsigned char suit_tile(unsigned char color) {
    switch (color) {
        case COLOR_RED: return TILE_RED;
        case COLOR_YELLOW: return TILE_YELLOW;
        case COLOR_GREEN: return TILE_GREEN;
        default: return TILE_BLUE;
    }
}

static unsigned char card_tile_color(unsigned char color, unsigned char color_override) {
    if (color == COLOR_WILD) {
        if (color_override == NONE) return TILE_WILD;
        return suit_tile(color_override);
    }
    return suit_tile(color);
}

/* Darkened-but-still-colored tile for a card that can't be legally played
   right now (see the TILE_*_DIM note in vbxevid.h). Wilds are always legal
   so never reach here, but the default keeps it total. */
static unsigned char dim_tile(unsigned char color) {
    switch (color) {
        case COLOR_RED: return TILE_RED_DIM;
        case COLOR_YELLOW: return TILE_YELLOW_DIM;
        case COLOR_GREEN: return TILE_GREEN_DIM;
        case COLOR_BLUE: return TILE_BLUE_DIM;
        default: return TILE_WILD_DIM;
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

/* Draws a solid 3x3 card tile at (x,y) in the given TILE_* color, with
   the card's value character centered (using the same color index, so
   it automatically renders in that tile's paired contrasting text
   color -- see vbxevid.c's palette table). */
static void draw_tile(unsigned char x, unsigned char y, unsigned char color, char value_ch) {
    scr_fill_rect(x, y, 3, 3, ' ', color);
    scr_put((unsigned char)(x + 1), (unsigned char)(y + 1), (unsigned char)value_ch, color);
}

/* Top-left corner of hand slot `i`'s 3x3 tile (shared by ui_draw_hand,
   ui_hand_card_pos, and ui_blink_cursor so all three agree on layout). */
static void hand_slot_pos(unsigned char i, unsigned char *x, unsigned char *y) {
    *x = (unsigned char)(1 + (i % CARDS_PER_ROW) * SLOT_W + 1);
    *y = (unsigned char)(HAND_Y + (i / CARDS_PER_ROW) * SLOT_H);
}

/* Draws the selection highlight for the tile at (x,y): a solid full-height
   colored bar down each side of the 3x3 tile (columns x-1 and x+3, rows
   y..y+2), far more visible than the old single-character brackets. A
   space cell painted in a TILE_* color shows as a solid block of that
   tile's background, so this reuses the same trick as draw_tile(). */
static void draw_sel_bars(unsigned char x, unsigned char y, unsigned char color) {
    scr_fill_rect((unsigned char)(x - 1), y, 1, 3, ' ', color);
    scr_fill_rect((unsigned char)(x + 3), y, 1, 3, ' ', color);
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
    scr_puts(15, 17, "CARDS SHOW AS COLORED TILES:", COL_WHITE);
    draw_tile(15, 18, TILE_RED, 'R');
    draw_tile(19, 18, TILE_YELLOW, 'Y');
    draw_tile(23, 18, TILE_GREEN, 'G');
    draw_tile(27, 18, TILE_BLUE, 'B');
    draw_tile(31, 18, TILE_WILD, 'W');
    scr_puts(11, 22, "S=SKIP V=REVERSE D=DRAW2 F=WILD+4", COL_WHITE);
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
    /* g->top_color is always an already-resolved plain color (0-3), even
       when the top card itself is a wild -- plain COL_* (black background,
       matching the rest of this status line), not the TILE_* tile-fill
       colors, which would put a colored patch behind this text. */
    static const char *names[4] = {"RED", "YEL", "GRN", "BLU"};
    static const unsigned char plain_colors[4] = {COL_RED, COL_YELLOW, COL_GREEN, COL_BLUE};
    unsigned char tile_col = card_tile_color(g->top_card.color, g->top_color);

    scr_fill_rect(0, TABLE_Y, COLS, 1, ' ', COL_BLACK);
    scr_puts(1, TABLE_Y, "DRAW PILE:", COL_WHITE);
    scr_put_num(12, TABLE_Y, g->draw_count, COL_WHITE);
    scr_puts(20, TABLE_Y, "COLOR:", COL_WHITE);
    scr_puts(27, TABLE_Y, names[g->top_color], plain_colors[g->top_color]);
    scr_puts(35, TABLE_Y, g->direction > 0 ? "DIR ->" : "DIR <-", COL_WHITE);

    draw_tile(TABLE_TILE_X, TABLE_Y, tile_col, value_char(g->top_card));
}

void ui_draw_hand(GameState *g, unsigned char cursor) {
    Player *p = &g->players[0];
    unsigned char i, x, y, shown, col;

    scr_fill_rect(0, HAND_LABEL_Y, COLS, 1, ' ', COL_BLACK);
    scr_puts(1, HAND_LABEL_Y, "YOUR HAND:", COL_WHITE);
    scr_put_num(12, HAND_LABEL_Y, p->count, COL_WHITE);

    scr_fill_rect(0, HAND_Y, COLS, 2 * SLOT_H, ' ', COL_BLACK);

    shown = (p->count > HAND_VISIBLE) ? HAND_VISIBLE : p->count;

    /* Dim any card that can't be legally played on the CURRENT top card,
       whoever's turn it is. Keying the dimming purely to (card, top card)
       -- rather than also to whose turn it is -- means a card's shade only
       changes when the top card actually changes, instead of the whole hand
       flipping bright/dim every time the turn passes between you and the
       CPUs (which read as distracting flicker). */
    for (i = 0; i < shown; i++) {
        unsigned char playable = is_legal(g, p->hand[i]);
        hand_slot_pos(i, &x, &y);
        col = playable ? card_tile_color(p->hand[i].color, NONE) : dim_tile(p->hand[i].color);
        draw_tile(x, y, col, value_char(p->hand[i]));
        scr_put((unsigned char)(x + 1), (unsigned char)(y + 3), (unsigned char)label_char(i), COL_WHITE);
        if (i == cursor) {
            draw_sel_bars(x, y, TILE_SELECTED);
        }
    }
}

void ui_message(const char *line1, const char *line2) {
    scr_fill_rect(0, MSG_Y1, COLS, 2, ' ', COL_BLACK);
    if (line1) scr_puts(1, MSG_Y1, line1, COL_WHITE);
    if (line2) scr_puts(1, MSG_Y2, line2, COL_WHITE);
}

void ui_draw_color_picker(unsigned char selected) {
    static const char *names[4] = {"RED", "YELLOW", "GREEN", "BLUE"};
    static const unsigned char tiles[4] = {TILE_RED, TILE_YELLOW, TILE_GREEN, TILE_BLUE};
    unsigned char i, x;

    scr_fill_rect(0, MSG_Y1, COLS, 2, ' ', COL_BLACK);
    scr_puts(1, MSG_Y1, "CHOOSE A COLOR:", COL_WHITE);
    for (i = 0; i < 4; i++) {
        unsigned char sel = (selected == i);
        x = 2 + i * 9;
        scr_put(x, MSG_Y2, sel ? '[' : ' ', COL_WHITE);
        scr_puts(x + 1, MSG_Y2, names[i], sel ? TILE_SELECTED : tiles[i]);
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
    scr_puts(x + 1, MSG_Y2, "YES", selected_yes ? TILE_SELECTED : COL_WHITE);
    scr_put(x + 4, MSG_Y2, selected_yes ? ']' : ' ', COL_WHITE);
    scr_put(x + 6, MSG_Y2, !selected_yes ? '[' : ' ', COL_WHITE);
    scr_puts(x + 7, MSG_Y2, "NO", !selected_yes ? TILE_SELECTED : COL_WHITE);
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

/* ---- Position helpers, blinking cursor, and win flourish (VBXE-only
   extras; see ui_vbxe.h) ---- */

void ui_hand_card_pos(unsigned char index, unsigned char *col, unsigned char *row) {
    hand_slot_pos(index, col, row);
}

void ui_draw_pile_pos(unsigned char *col, unsigned char *row) {
    *col = 12;
    *row = TABLE_Y;
}

void ui_top_card_pos(unsigned char *col, unsigned char *row) {
    *col = TABLE_TILE_X;
    *row = TABLE_Y;
}

void ui_opponent_pos(unsigned char idx, unsigned char *col, unsigned char *row) {
    *col = (unsigned char)(2 + (idx - 1) * 20 + 5);
    *row = OPP_Y;
}

unsigned char ui_suit_color(unsigned char color) {
    return suit_tile(color);
}

/* Pulses the selection bars between bright (TILE_SELECTED) and dim
   (TILE_DIM) rather than fully on/off, so the selected card is ALWAYS
   clearly marked -- it just breathes to draw the eye, instead of vanishing
   for half the blink cycle the way the old single brackets did. */
void ui_blink_cursor(unsigned char cursor, unsigned char on) {
    unsigned char x, y;
    hand_slot_pos(cursor, &x, &y);
    draw_sel_bars(x, y, on ? TILE_SELECTED : TILE_SEL_DIM);
}

void ui_win_flourish_step(unsigned char step) {
    static const unsigned char colors[6] = {
        COL_RED, COL_YELLOW, COL_GREEN, COL_CYAN, COL_BLUE, COL_MAGENTA
    };
    unsigned char x;
    for (x = 0; x < COLS; x++) {
        unsigned char c = colors[(x + step) % 6];
        scr_put(x, 5, '*', c);
        scr_put(x, 13, '*', c);
    }
}
