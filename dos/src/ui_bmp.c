/* Bitmap UNO UI for the DOS graphics builds. Implements the same ui.h
   interface as the CGA text build, but draws pixel-art cards through gfx.h
   -- which is backed by either EGA mode 10h (640x350x16) or VGA mode 13h
   (320x200x256).

   Every position below is derived from the GFX_W, GFX_H, GFX_CARD and GFX_FONT sizes
   rather than written as a literal, because those two modes differ by a
   factor of two in each axis. One layout, two very different screens. */
#include <string.h>
#include "game.h"
#include "cards.h"
#include "ui.h"
#include "gfx.h"

#define FW GFX_FONT_W
#define FH GFX_FONT_H

#define TITLE_Y      4
#define OPP_Y        (TITLE_Y + FH + 6)
#define TBL_LABEL_Y  (OPP_Y + FH + 8)
#define CARD_Y       (TBL_LABEL_Y + FH + 2)
#define MSG_Y1       (CARD_Y + GFX_CARD_H + 8)
#define MSG_Y2       (MSG_Y1 + FH + 2)
#define HAND_Y       (MSG_Y2 + FH + 8)
#define HAND_STRIDE  (GFX_CARD_H + 6)
#define HELP_Y       (GFX_H - FH - 2)

#define CARD_STEP    (GFX_W / HAND_PER_ROW)

/* Each table column is as wide as the wider of its card and its label.
   Neither alone is enough: CARD_STEP is the hand's spacing (32 pixels at
   320, which ran "DRAW" into "TOP CARD"), and the card width is narrower
   still -- 28 pixels against the 56 that "DRAW 79" needs. At 640 the cards
   are the wider of the two and at 320 the labels are, so the column has to
   take the larger. */
#define WIDER(a, b)  ((a) > (b) ? (a) : (b))
#define DRAW_LABEL_W (7 * FW)          /* "DRAW nn" */
#define TOP_LABEL_W  (8 * FW)          /* "TOP CARD" */

#define DRAW_X       8
#define TOP_X        (DRAW_X + WIDER(GFX_CARD_W, DRAW_LABEL_W) + 8)
#define INFO_X       (TOP_X + WIDER(GFX_CARD_W, TOP_LABEL_W) + 8)

/* How many characters fit across. This is the one thing that does *not*
   scale with resolution: the ROM font is 8 pixels wide in both modes, so
   EGA gets 80 columns and VGA only 40. Vertical layout can be derived from
   GFX_H, but anything text-heavy has to know which it is on -- three
   opponent labels reading "CPU1 CARDS: 7" need 39 columns of the 40 VGA
   has, and the help line needs more than it has at all. */
#define GFX_COLS (GFX_W / FW)
#define WIDE_SCREEN (GFX_COLS >= 60)

/* The colour picker needs a taller clear than an ordinary two-line message.
   Its selection frame starts at MSG_Y2-3 and stands FH+6 tall, so it
   reaches MSG_Y1 + 2*FH + 5 -- past the 2*FH+2 a message occupies. Nothing
   else covers those rows either: ui_draw_hand() starts its own clear at
   HAND_Y-4, which is lower still, so the bottom of the frame was surviving
   as a stray white line under the message area. */
#define MSG_CLR_H    (2 * FH + 2)
#define PICKER_CLR_H (2 * FH + 8)

static const unsigned char suit_col[5] = {
    GC_LRED, GC_YELLOW, GC_LGREEN, GC_LBLUE, GC_DGRAY
};
/* What to write *on* each suit colour. White is unreadable on yellow and
   poor on the bright green, so those two take black -- the CGA build has
   carried this table from the start and the bitmap UI should have too. */
static const unsigned char suit_fg[5] = {
    GC_WHITE, GC_BLACK, GC_BLACK, GC_WHITE, GC_WHITE
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

static void put_num(int x, int y, unsigned int n, unsigned char col)
{
    char buf[6];
    int i = 0, j;
    if (n == 0) { gfx_text_t(x, y, "0", col); return; }
    while (n && i < 5) { buf[i++] = (char)('0' + (n % 10)); n /= 10; }
    for (j = 0; j < i; j++) {
        char one[2];
        one[0] = buf[i - 1 - j];
        one[1] = '\0';
        gfx_text_t(x + j * FW, y, one, col);
    }
}

static void card_face(int x, int y, unsigned char suit, unsigned char value)
{
    unsigned char s = (suit <= 4) ? suit : 4;
    unsigned char c = suit_col[s];
    char v[2];
    int bw = FW + 8, bh = FH + 6;

    v[0] = value_char(value);
    v[1] = '\0';

    gfx_fill_rect(x + 3, y + 3, GFX_CARD_W, GFX_CARD_H, GC_SHADOW);
    gfx_fill_rect(x, y, GFX_CARD_W, GFX_CARD_H, c);
    gfx_fill_rect(x + 3, y + 3, GFX_CARD_W - 6, GFX_CARD_H - 6, GC_WHITE);

    gfx_char(x + 4, y + 4, v[0], c, GC_WHITE);
    gfx_char(x + GFX_CARD_W - 4 - FW, y + GFX_CARD_H - 4 - FH, v[0], c, GC_WHITE);

    gfx_fill_rect(x + (GFX_CARD_W - bw) / 2, y + (GFX_CARD_H - bh) / 2, bw, bh, c);
    gfx_char(x + (GFX_CARD_W - FW) / 2, y + (GFX_CARD_H - FH) / 2, v[0],
             suit_fg[s], c);
}

static void card_back(int x, int y)
{
    gfx_fill_rect(x + 3, y + 3, GFX_CARD_W, GFX_CARD_H, GC_SHADOW);
    gfx_fill_rect(x, y, GFX_CARD_W, GFX_CARD_H, GC_WHITE);
    gfx_fill_rect(x + 3, y + 3, GFX_CARD_W - 6, GFX_CARD_H - 6, GC_RED);
    gfx_text(x + (GFX_CARD_W - 3 * FW) / 2, y + (GFX_CARD_H - FH) / 2,
             "UNO", GC_WHITE, GC_RED);
}

static void clear_band(int y, int h)
{
    gfx_fill_rect(0, y, GFX_W, h, GC_FELT);
}

static int player_label(int x, int y, unsigned char idx, unsigned char col)
{
    if (idx == 0) { gfx_text_t(x, y, "YOU", col); return x + 4 * FW; }
    gfx_text_t(x, y, "CPU", col);
    put_num(x + 3 * FW, y, idx, col);
    return x + 5 * FW;
}

void ui_title_screen(void)
{
    int cx = GFX_W / 2;
    gfx_clear(GC_FELT);
    gfx_text_t(cx - 3 * FW, TITLE_Y + FH, "U N O", GC_YELLOW);
    gfx_text_t(cx - 7 * FW, TITLE_Y + 3 * FH, "1 PLAYER VS 3 CPU", GC_WHITE);

    gfx_text_t(cx - 13 * FW, TITLE_Y + 6 * FH, "LEFT / RIGHT  - PICK A CARD", GC_WHITE);
    gfx_text_t(cx - 13 * FW, TITLE_Y + 7 * FH, "SPACE / ENTER - PLAY A CARD", GC_WHITE);
    gfx_text_t(cx - 13 * FW, TITLE_Y + 8 * FH, "U or UP       - DRAW A CARD", GC_WHITE);
    gfx_text_t(cx - 13 * FW, TITLE_Y + 9 * FH, "ESC           - QUIT TO DOS", GC_WHITE);

    gfx_text_t(cx - 10 * FW, TITLE_Y + 11 * FH, "PRESS SPACE TO START", GC_YELLOW);

    card_face(cx - CARD_STEP - GFX_CARD_W / 2, HAND_Y, 0, 5);
    card_face(cx - GFX_CARD_W / 2, HAND_Y, 1, 7);
    card_face(cx + CARD_STEP - GFX_CARD_W / 2, HAND_Y, 4, VAL_WILD);
}

void ui_draw_frame(void)
{
    gfx_clear(GC_FELT);
    gfx_text_t(GFX_W / 2 - 3 * FW, TITLE_Y, "U N O", GC_YELLOW);
#if WIDE_SCREEN
    gfx_text_t(8, HELP_Y, "ARROWS PICK   SPACE PLAYS   U DRAWS   ESC QUITS", GC_LGRAY);
#else
    gfx_text_t(8, HELP_Y, "ARROWS  SPACE  U DRAWS  ESC", GC_LGRAY);
#endif
}

void ui_draw_opponents(GameState *g)
{
    unsigned char opp;
    clear_band(OPP_Y, FH);
    for (opp = 1; opp <= 3; opp++) {
        int x = 8 + (int)(opp - 1) * (GFX_W / 3);
        unsigned char col = (g->current_player == opp) ? GC_YELLOW : GC_WHITE;
        int nx = player_label(x, OPP_Y, opp, col);
#if WIDE_SCREEN
        gfx_text_t(nx, OPP_Y, "CARDS:", col);
        put_num(nx + 7 * FW, OPP_Y, g->players[opp].count, col);
#else
        /* "CPUn:N" -- three of these are 18 columns of the 40 available,
           where the spelled-out form needs 39 and collides. */
        gfx_text_t(nx - FW, OPP_Y, ":", col);
        put_num(nx, OPP_Y, g->players[opp].count, col);
#endif
    }
}

void ui_draw_table(GameState *g)
{
    clear_band(TBL_LABEL_Y, CARD_Y + GFX_CARD_H + 6 - TBL_LABEL_Y);

    gfx_text_t(DRAW_X, TBL_LABEL_Y, "DRAW", GC_LGRAY);
    put_num(DRAW_X + 5 * FW, TBL_LABEL_Y, g->draw_count, GC_LGRAY);
    card_back(DRAW_X, CARD_Y);

    gfx_text_t(TOP_X, TBL_LABEL_Y, "TOP CARD", GC_LGRAY);
    card_face(TOP_X, CARD_Y, g->top_card.color, g->top_card.value);

    gfx_text_t(INFO_X, CARD_Y, "COLOR:", GC_WHITE);
    gfx_text_t(INFO_X, CARD_Y + FH + 2, color_name[g->top_color],
               suit_col[g->top_color]);
    gfx_text_t(INFO_X, CARD_Y + 3 * FH, g->direction > 0 ? "DIR ->" : "DIR <-",
               GC_WHITE);
}

void ui_draw_hand(GameState *g, unsigned char cursor)
{
    Player *p = &g->players[0];
    unsigned char i, n = p->count > HAND_VISIBLE ? HAND_VISIBLE : p->count;

    clear_band(HAND_Y - 4, HELP_Y - (HAND_Y - 4));
    for (i = 0; i < n; i++) {
        int col = i % HAND_PER_ROW;
        int row = i / HAND_PER_ROW;
        int x = col * CARD_STEP + 4;
        int y = HAND_Y + row * HAND_STRIDE;

        card_face(x, y, p->hand[i].color, p->hand[i].value);
        if (i == cursor) {
            gfx_frame_rect(x - 2, y - 2, GFX_CARD_W + 4, GFX_CARD_H + 4, GC_YELLOW);
            gfx_frame_rect(x - 1, y - 1, GFX_CARD_W + 2, GFX_CARD_H + 2, GC_YELLOW);
        }
    }
}

void ui_message(const char *line1, const char *line2)
{
    clear_band(MSG_Y1, MSG_CLR_H);
    if (line1) gfx_text_t(8, MSG_Y1, line1, GC_WHITE);
    if (line2) gfx_text_t(8, MSG_Y2, line2, GC_WHITE);
}

void ui_draw_color_picker(unsigned char selected)
{
    unsigned char i;
    clear_band(MSG_Y1, PICKER_CLR_H);
    gfx_text_t(8, MSG_Y1, "CHOOSE A COLOR:", GC_WHITE);
    for (i = 0; i < 4; i++) {
        int x = 8 + (int)i * (GFX_W / 5);
        gfx_fill_rect(x, MSG_Y2, 6 * FW, FH, suit_col[i]);
        gfx_text(x + FW, MSG_Y2, color_name[i], suit_fg[i], suit_col[i]);
        /* The highlight is white, not yellow: yellow is one of the four
           choices, and a yellow frame around the yellow swatch is invisible
           -- which makes the picker look like it is not responding at all.
           Arrow markers either side as well, so the selection reads even
           where the frame is subtle. */
        if (i == selected) {
            gfx_frame_rect(x - 3, MSG_Y2 - 3, 6 * FW + 6, FH + 6, GC_WHITE);
            gfx_frame_rect(x - 2, MSG_Y2 - 2, 6 * FW + 4, FH + 4, GC_WHITE);
            gfx_text_t(x - 2 * FW, MSG_Y2, ">", GC_WHITE);
            gfx_text_t(x + 6 * FW + FW, MSG_Y2, "<", GC_WHITE);
        }
    }
}

void ui_clear_color_picker(void)
{
    clear_band(MSG_Y1, PICKER_CLR_H);
}

void ui_game_over_screen(unsigned char human_won, unsigned char winner_idx)
{
    int cx = GFX_W / 2;
    gfx_clear(GC_FELT);
    if (human_won) {
        gfx_text_t(cx - 4 * FW, GFX_H / 3, "YOU WIN!", GC_YELLOW);
        gfx_text_t(cx - 10 * FW, GFX_H / 3 + 2 * FH, "GREAT GAME, CHAMPION.", GC_WHITE);
    } else {
        gfx_text_t(cx - 4 * FW, GFX_H / 3, "GAME OVER", GC_LRED);
        gfx_text_t(cx - 4 * FW, GFX_H / 3 + 2 * FH, "CPU", GC_WHITE);
        put_num(cx - FW, GFX_H / 3 + 2 * FH, winner_idx, GC_WHITE);
        gfx_text_t(cx + FW, GFX_H / 3 + 2 * FH, "WINS", GC_WHITE);
    }
    gfx_text_t(cx - 12 * FW, GFX_H / 2 + 2 * FH, "PRESS SPACE TO PLAY AGAIN", GC_WHITE);
    gfx_text_t(cx - 7 * FW, GFX_H / 2 + 3 * FH, "OR ESC TO QUIT", GC_LGRAY);
}

void ui_draw_challenge_prompt(unsigned char victim, unsigned char player,
                              unsigned char selected_yes)
{
    int lx;
    (void)victim;
    clear_band(MSG_Y1, MSG_CLR_H);
    lx = player_label(8, MSG_Y1, player, GC_WHITE);
    gfx_text_t(lx, MSG_Y1, "PLAYED WILD DRAW FOUR", GC_WHITE);
    gfx_text_t(8, MSG_Y2, "CHALLENGE?", GC_WHITE);
    gfx_text_t(8 + 12 * FW, MSG_Y2, "YES", selected_yes ? GC_YELLOW : GC_LGRAY);
    gfx_text_t(8 + 17 * FW, MSG_Y2, "NO", selected_yes ? GC_LGRAY : GC_YELLOW);
    gfx_text_t(8 + 21 * FW, MSG_Y2, "SPACE/RET", GC_LGRAY);
}

void ui_event_challenge_result(unsigned char victim, unsigned char player,
                               unsigned char succeeded)
{
    int lx;
    clear_band(MSG_Y1, MSG_CLR_H);
    lx = player_label(8, MSG_Y1, victim, GC_WHITE);
    gfx_text_t(lx, MSG_Y1, "CHALLENGES!", GC_WHITE);
    if (succeeded) {
        lx = player_label(8, MSG_Y2, player, GC_LRED);
        gfx_text_t(lx, MSG_Y2, "HAD A MATCH - DRAWS 4", GC_LRED);
    } else {
        lx = player_label(8, MSG_Y2, victim, GC_LRED);
        gfx_text_t(lx, MSG_Y2, "WAS WRONG - DRAWS 6", GC_LRED);
    }
}

void ui_event_skip(unsigned char idx)
{
    int lx;
    clear_band(MSG_Y1, MSG_CLR_H);
    lx = player_label(8, MSG_Y1, idx, GC_WHITE);
    gfx_text_t(lx, MSG_Y1, idx == 0 ? "LOSE A TURN" : "IS SKIPPED", GC_WHITE);
}

void ui_event_reverse(unsigned char idx)
{
    (void)idx;
    clear_band(MSG_Y1, MSG_CLR_H);
    gfx_text_t(8, MSG_Y1, "REVERSE! ORDER FLIPPED", GC_WHITE);
}

void ui_event_draw(unsigned char idx, unsigned char count)
{
    const char *w = idx == 0 ? "MUST DRAW" : "DRAWS";
    int lx;
    clear_band(MSG_Y1, MSG_CLR_H);
    lx = player_label(8, MSG_Y1, idx, GC_WHITE);
    gfx_text_t(lx, MSG_Y1, w, GC_WHITE);
    put_num(lx + ((int)strlen(w) + 1) * FW, MSG_Y1, count, GC_WHITE);
}

void ui_event_uno(unsigned char idx)
{
    int lx;
    clear_band(MSG_Y2, FH);
    lx = player_label(8, MSG_Y2, idx, GC_YELLOW);
    gfx_text_t(lx, MSG_Y2, "UNO! ONE CARD LEFT!", GC_YELLOW);
}

void ui_event_invalid(void)
{
    clear_band(MSG_Y2, FH);
    gfx_text_t(8, MSG_Y2, "THAT CARD DOES NOT MATCH!", GC_LRED);
}

void ui_event_drew_one(unsigned char idx)
{
    int lx;
    clear_band(MSG_Y1, MSG_CLR_H);
    lx = player_label(8, MSG_Y1, idx, GC_WHITE);
    gfx_text_t(lx, MSG_Y1, "DREW A CARD", GC_WHITE);
}

void ui_event_thinking(unsigned char idx)
{
    int lx;
    clear_band(MSG_Y1, MSG_CLR_H);
    lx = player_label(8, MSG_Y1, idx, GC_WHITE);
    gfx_text_t(lx, MSG_Y1, "IS THINKING...", GC_WHITE);
}
