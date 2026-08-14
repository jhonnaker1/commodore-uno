/* UNO's screen for MS-DOS, in CGA 80x25 colour text.

   Cards are solid colour tiles -- the same approach as the X16, VBXE, F256
   and MEGA65 ports, since CGA gives per-cell foreground and background just
   as those do. What CGA adds over those machines is width: 80 columns is
   twice the C64's 40 and two and a half times the MSX2's 32, so the whole
   hand fits as two straight rows of ten with no overlapping fan. */
#include <string.h>
#include "game.h"
#include "cards.h"
#include "ui.h"
#include "cgavid.h"

#define TITLE_X 37
#define TITLE_Y 0
#define OPP_Y 2
#define TBL_LABEL_Y 4
#define TBL_CARD_Y 5
#define DRAW_X 2
#define TOP_X 12
#define INFO_X 24
#define MSG_Y1 11
#define MSG_Y2 12
#define HAND_LABEL_Y 13
#define HAND_Y 14
#define HAND_ROW_STRIDE 5      /* 4 rows of card + 1 row of cursor marker */

#define CARD_W 7
#define CARD_H 4
#define CARD_STEP 8

#define BG ATTR(C_LGRAY, C_BLACK)
#define A_WHITE ATTR(C_WHITE, C_BLACK)
#define A_GRAY  ATTR(C_LGRAY, C_BLACK)
#define A_YEL   ATTR(C_YELLOW, C_BLACK)
#define A_RED   ATTR(C_LRED, C_BLACK)

/* Suit tile colours. Backgrounds above 7 are only legal because cga_init()
   turns blinking off -- see cgavid.h. Yellow and green take a black
   foreground because white on either is close to unreadable. */
static const unsigned char suit_bg[5] = {
    C_LRED, C_YELLOW, C_LGREEN, C_LBLUE, C_DGRAY
};
static const unsigned char suit_fg[5] = {
    C_WHITE, C_BLACK, C_BLACK, C_WHITE, C_WHITE
};
/* For plain text in a suit's colour, on the normal black background. */
static const unsigned char suit_text[5] = {
    C_LRED, C_YELLOW, C_LGREEN, C_LBLUE, C_LGRAY
};
static const char *const color_name[4] = {"RED", "YELLOW", "GREEN", "BLUE"};

static char value_char(unsigned char value)
{
    switch (value) {
    case VAL_SKIP:    return 'S';
    case VAL_REVERSE: return 'R';
    case VAL_DRAW2:   return 'D';
    case VAL_WILD:    return 'W';
    case VAL_WILD4:   return 'F';
    default:          return (char)('0' + value);
    }
}

static void draw_card_face(int x, int y, unsigned char suit, unsigned char value)
{
    unsigned char s = (suit <= 4) ? suit : 4;
    unsigned char a = ATTR(suit_fg[s], suit_bg[s]);
    char v = value_char(value);

    scr_fill(x, y, CARD_W, CARD_H, ' ', a);
    scr_put(x + 1, y, v, a);
    scr_put(x + 3, y + 1, v, a);
    scr_put(x + 5, y + 3, v, a);
}

static void draw_card_back(int x, int y)
{
    unsigned char a = ATTR(C_WHITE, C_RED);
    scr_fill(x, y, CARD_W, CARD_H, ' ', a);
    scr_puts(x + 2, y + 1, "UNO", a);
}

static void clear_rows(int y, int h)
{
    scr_fill(0, y, COLS, h, ' ', BG);
}

/* Draw a player's label and return the x the following text starts at. */
static int player_label(int x, int y, unsigned char idx, unsigned char attr)
{
    if (idx == 0) { scr_puts(x, y, "YOU", attr); return x + 4; }
    scr_puts(x, y, "CPU", attr);
    scr_put(x + 3, y, (char)('0' + idx), attr);
    return x + 5;
}

void ui_title_screen(void)
{
    scr_clear(BG);
    scr_puts(TITLE_X, 2, "U N O", A_YEL);
    scr_puts(32, 4, "MS-DOS EDITION", A_WHITE);
    scr_puts(27, 6, "1 PLAYER VS 3 CPU PLAYERS", A_GRAY);

    scr_puts(24, 9,  "LEFT / RIGHT  - PICK A CARD", A_WHITE);
    scr_puts(24, 10, "SPACE / ENTER - PLAY A CARD", A_WHITE);
    scr_puts(24, 11, "U or UP       - DRAW A CARD", A_WHITE);
    scr_puts(24, 12, "1-9 0 A-J     - JUMP TO A SLOT", A_WHITE);
    scr_puts(24, 13, "ESC           - QUIT TO DOS", A_WHITE);

    scr_puts(28, 16, "PRESS SPACE TO START", A_YEL);

    draw_card_face(28, 19, 0, 5);
    draw_card_face(36, 19, 1, 7);
    draw_card_face(44, 19, 4, VAL_WILD);
}

void ui_draw_frame(void)
{
    scr_clear(BG);
    scr_puts(TITLE_X, TITLE_Y, "U N O", A_YEL);
    scr_puts(2, 24, "ARROWS PICK   SPACE PLAYS   U DRAWS   ESC QUITS", A_GRAY);
}

void ui_draw_opponents(GameState *g)
{
    unsigned char opp;
    clear_rows(OPP_Y, 1);
    for (opp = 1; opp <= 3; opp++) {
        int x = 4 + (int)(opp - 1) * 26;
        unsigned char a = (g->current_player == opp) ? A_YEL : A_WHITE;
        int nx = player_label(x, OPP_Y, opp, a);
        scr_puts(nx, OPP_Y, "CARDS:", a);
        scr_put_num(nx + 7, OPP_Y, g->players[opp].count, a);
    }
}

void ui_draw_table(GameState *g)
{
    clear_rows(TBL_LABEL_Y, TBL_CARD_Y + CARD_H - TBL_LABEL_Y + 1);

    scr_puts(DRAW_X, TBL_LABEL_Y, "DRAW", A_GRAY);
    scr_put_num(DRAW_X + 5, TBL_LABEL_Y, g->draw_count, A_GRAY);
    draw_card_back(DRAW_X, TBL_CARD_Y);

    scr_puts(TOP_X, TBL_LABEL_Y, "TOP CARD", A_GRAY);
    draw_card_face(TOP_X, TBL_CARD_Y, g->top_card.color, g->top_card.value);

    scr_puts(INFO_X, TBL_CARD_Y, "COLOR:", A_WHITE);
    scr_puts(INFO_X + 7, TBL_CARD_Y, color_name[g->top_color],
             ATTR(suit_text[g->top_color], C_BLACK));
    scr_puts(INFO_X, TBL_CARD_Y + 2, g->direction > 0 ? "DIRECTION: ->"
                                                      : "DIRECTION: <-", A_WHITE);
}

void ui_draw_hand(GameState *g, unsigned char cursor)
{
    Player *p = &g->players[0];
    unsigned char i, n = p->count > HAND_VISIBLE ? HAND_VISIBLE : p->count;

    clear_rows(HAND_LABEL_Y, ROWS - 1 - HAND_LABEL_Y);
    scr_puts(2, HAND_LABEL_Y, "YOUR HAND", A_GRAY);
    scr_put_num(12, HAND_LABEL_Y, p->count, A_GRAY);

    for (i = 0; i < n; i++) {
        int col = i % HAND_PER_ROW;
        int row = i / HAND_PER_ROW;
        int x = col * CARD_STEP;
        int y = HAND_Y + row * HAND_ROW_STRIDE;
        unsigned char s = p->hand[i].color <= 4 ? p->hand[i].color : 4;
        char slot;

        draw_card_face(x, y, p->hand[i].color, p->hand[i].value);

        /* The quick-play key, in the tile's bottom-left corner. It can't go
           on the row above: that is HAND_LABEL_Y, and the rows here are
           packed tight enough that there is no spare one. Bottom-left rather
           than top-left because the value glyphs sit at x+1, x+3 and x+5 --
           so the top-left cell is free but immediately abuts a value, while
           the bottom-left is a corner away from all three. */
        if (i < 9) slot = (char)('1' + i);
        else if (i == 9) slot = '0';
        else slot = (char)('A' + i - 10);
        scr_put(x, y + CARD_H - 1, slot, ATTR(suit_fg[s], suit_bg[s]));

        if (i == cursor)
            scr_fill(x, y + CARD_H, CARD_W, 1, (char)0x1E, A_YEL);
    }
}

void ui_message(const char *line1, const char *line2)
{
    clear_rows(MSG_Y1, 2);
    if (line1) scr_puts(2, MSG_Y1, line1, A_WHITE);
    if (line2) scr_puts(2, MSG_Y2, line2, A_WHITE);
}

void ui_draw_color_picker(unsigned char selected)
{
    unsigned char i;
    clear_rows(MSG_Y1, 2);
    scr_puts(2, MSG_Y1, "CHOOSE A COLOR:", A_WHITE);
    for (i = 0; i < 4; i++) {
        int x = 2 + (int)i * 18;
        unsigned char a = ATTR(suit_fg[i], suit_bg[i]);
        scr_fill(x, MSG_Y2, 8, 1, ' ', a);
        scr_puts(x + 1, MSG_Y2, color_name[i], a);
        if (i == selected) {
            scr_put(x - 1, MSG_Y2, (char)0x10, A_YEL);   /* > */
            scr_put(x + 8, MSG_Y2, (char)0x11, A_YEL);   /* < */
        }
    }
    scr_puts(74, MSG_Y1, "SPACE", A_GRAY);
}

void ui_clear_color_picker(void)
{
    clear_rows(MSG_Y1, 2);
}

void ui_game_over_screen(unsigned char human_won, unsigned char winner_idx)
{
    scr_clear(BG);
    if (human_won) {
        scr_puts(36, 9, "YOU WIN!", A_YEL);
        scr_puts(29, 11, "GREAT GAME, CHAMPION.", A_WHITE);
    } else {
        scr_puts(35, 9, "GAME OVER", A_RED);
        scr_puts(35, 11, "CPU", A_WHITE);
        scr_put(38, 11, (char)('0' + winner_idx), A_WHITE);
        scr_puts(40, 11, "WINS", A_WHITE);
    }
    scr_puts(27, 15, "PRESS SPACE TO PLAY AGAIN", A_WHITE);
    scr_puts(31, 16, "OR ESC TO QUIT", A_GRAY);
}

/* ---- event / prompt lines ---- */

void ui_draw_challenge_prompt(unsigned char victim, unsigned char player,
                              unsigned char selected_yes)
{
    int lx;
    (void)victim;
    clear_rows(MSG_Y1, 2);
    lx = player_label(2, MSG_Y1, player, A_WHITE);
    scr_puts(lx, MSG_Y1, "PLAYED WILD DRAW FOUR", A_WHITE);

    scr_puts(2, MSG_Y2, "CHALLENGE?", A_WHITE);
    scr_puts(14, MSG_Y2, "YES", selected_yes ? A_YEL : A_GRAY);
    scr_puts(20, MSG_Y2, "NO", selected_yes ? A_GRAY : A_YEL);
    scr_puts(26, MSG_Y2, "(LEFT/RIGHT, THEN SPACE)", A_GRAY);
}

void ui_event_challenge_result(unsigned char victim, unsigned char player,
                               unsigned char succeeded)
{
    int lx;
    clear_rows(MSG_Y1, 2);
    lx = player_label(2, MSG_Y1, victim, A_WHITE);
    scr_puts(lx, MSG_Y1, "CHALLENGES!", A_WHITE);
    if (succeeded) {
        lx = player_label(2, MSG_Y2, player, A_RED);
        scr_puts(lx, MSG_Y2, "HAD A MATCH - DRAWS 4", A_RED);
    } else {
        lx = player_label(2, MSG_Y2, victim, A_RED);
        scr_puts(lx, MSG_Y2, "WAS WRONG - DRAWS 6", A_RED);
    }
}

void ui_event_skip(unsigned char idx)
{
    int lx;
    clear_rows(MSG_Y1, 2);
    lx = player_label(2, MSG_Y1, idx, A_WHITE);
    scr_puts(lx, MSG_Y1, idx == 0 ? "LOSE A TURN" : "IS SKIPPED", A_WHITE);
}

void ui_event_reverse(unsigned char idx)
{
    (void)idx;
    clear_rows(MSG_Y1, 2);
    scr_puts(2, MSG_Y1, "REVERSE! ORDER FLIPPED", A_WHITE);
}

void ui_event_draw(unsigned char idx, unsigned char count)
{
    const char *w = idx == 0 ? "MUST DRAW" : "DRAWS";
    int lx;
    clear_rows(MSG_Y1, 2);
    lx = player_label(2, MSG_Y1, idx, A_WHITE);
    scr_puts(lx, MSG_Y1, w, A_WHITE);
    scr_put_num(lx + (int)strlen(w) + 1, MSG_Y1, count, A_WHITE);
}

void ui_event_uno(unsigned char idx)
{
    int lx;
    clear_rows(MSG_Y2, 1);
    lx = player_label(2, MSG_Y2, idx, A_YEL);
    scr_puts(lx, MSG_Y2, "UNO! ONE CARD LEFT!", A_YEL);
}

void ui_event_invalid(void)
{
    clear_rows(MSG_Y2, 1);
    scr_puts(2, MSG_Y2, "THAT CARD DOES NOT MATCH!", A_RED);
}

void ui_event_drew_one(unsigned char idx)
{
    int lx;
    clear_rows(MSG_Y1, 2);
    lx = player_label(2, MSG_Y1, idx, A_WHITE);
    scr_puts(lx, MSG_Y1, "DREW A CARD", A_WHITE);
}

void ui_event_thinking(unsigned char idx)
{
    int lx;
    clear_rows(MSG_Y1, 2);
    lx = player_label(2, MSG_Y1, idx, A_WHITE);
    scr_puts(lx, MSG_Y1, "IS THINKING...", A_WHITE);
}
