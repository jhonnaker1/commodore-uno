/* UNO for C64 OS, in C.
 *
 * The same windowed application as ../c64os (menu bar, event-driven
 * keyboard, the OS's own draw cycle), but the game rules are the shared
 * cards.c/game.c/ai.c every other C port compiles -- byte-identical, and
 * enforced by `make check` at the repo root -- instead of a hand port to
 * assembly. llvm-mos compiles it; src/app.s is the C64 OS side: the
 * application header, the layer, the KERNAL link table, and the
 * trampolines that keep the OS's zero page and C's apart.
 *
 * Drawing is direct to C64 OS's screen and colour buffers ($0400/$D800),
 * which is what the assembly port's layer blit does too. Nothing here
 * makes a syscall per character.
 */
#include <stdlib.h>
#include "cards.h"
#include "game.h"
#include "ai.h"
#include "sid.h"

/* ---- the OS side (src/app.s) --------------------------------------- */

void os_markredraw(void);
void os_quitapp(void);
unsigned int os_readkprnt(void); /* key in the low byte; high byte 1 = empty */
void os_deqkprnt(void);
void os_present(void); /* copy C64 OS's screen buffers to the screen now */

/* Message commands and the menu action codes from src/menu.json. The
 * menu file is PETSCII, so 'n' there is $4E, not ASCII's $6E. */
#define MC_MNU 0x00
#define MC_MENQ 0x01
#define ACT_NEW 0x4E  /* "n" */
#define ACT_QUIT 0x21 /* "!" */

/* Returned to app.s's msgcmd: A in the low byte, carry in bit 8. */
#define MSG_HANDLED(a) ((unsigned int)(a))
#define MSG_UNHANDLED 0x0100u

#define SCRBUF ((unsigned char *)0x0400)
#define COLBUF ((unsigned char *)0xD800)
#define COLS 40

/* The KERNAL jiffy clock, which C64 OS leaves running (measured by the
 * ega trek scope: $A1/$A2 advance while C64 OS idles). */
#define JIFFY_MID (*(volatile unsigned char *)0xA1)
#define JIFFY_LO (*(volatile unsigned char *)0xA2)

/* C64 colour numbers, as C64 OS's colors.s names them. */
#define C_BLACK 0
#define C_RED 2
#define C_GREEN 5
#define C_BLUE 6
#define C_YELLOW 7
#define C_LBLUE 14

/* PETSCII from the key queue. */
#define K_RETURN 0x0D
#define K_SPACE 0x20
#define K_LEFT 0x9D
#define K_RIGHT 0x1D
#define K_UP 0x91

/* Rows. 0 is C64 OS's menu bar and 24 its status bar: never drawn. */
#define ROW_OPP 1
#define ROW_TABLE 3
#define ROW_STATUS 5
#define ROW_PROMPT 6
#define ROW_EVENT 7
#define ROW_HAND_LABEL 8
#define ROW_HAND 9
#define ROW_KEYS 20
#define FIRST_ROW 1
#define LAST_ROW 23

#define CARDS_PER_ROW 6
#define QUICK_KEYS 20 /* 1-9, 0, A-J */

/* Pacing, in jiffies: long enough to read each CPU turn, short enough
 * that a lap of three CPU turns stays around two seconds. */
#define PAUSE_THINK 12
#define PAUSE_SHOW 30

enum { ST_TITLE, ST_HAND, ST_COLOR, ST_CHALLENGE, ST_OVER };

static GameState g;
static unsigned char state = ST_TITLE;
static unsigned char cursor;
static unsigned char pending_idx; /* hand slot waiting on the colour picker */
static unsigned char color_sel;
static unsigned char challenge_sel; /* 0 = yes, 1 = no */
static unsigned char invalid_flash;
static unsigned char thinking = NONE; /* CPU whose turn is being shown */
static unsigned int seed = 1;

#define LINE_LEN 38
static char event_line[LINE_LEN + 1]; /* what just happened */
static char effect_line[LINE_LEN + 1]; /* and what it did */

/* ---- drawing -------------------------------------------------------- */

static unsigned char ink = C_BLACK;
static unsigned char rvs;
static unsigned char *scr;
static unsigned char *col;

static void at(unsigned char row, unsigned char x)
{
    unsigned int off = (unsigned int)row * COLS + x;
    scr = SCRBUF + off;
    col = COLBUF + off;
}

/* ASCII to screen code, in C64 OS's (lowercase) character set: lowercase
 * letters are 1-26, capitals 65-90, punctuation and digits as-is. */
static unsigned char scrcode(char ch)
{
    unsigned char c = (unsigned char)ch;
    if (c >= 'a' && c <= 'z')
        return (unsigned char)(c - 'a' + 1);
    if (c >= 'A' && c <= 'Z')
        return (unsigned char)(c - 'A' + 65);
    if (c == '@')
        return 0;
    if (c == '[')
        return 27;
    if (c == ']')
        return 29;
    return c;
}

static void putch(char ch)
{
    *scr++ = (unsigned char)(scrcode(ch) | rvs);
    *col++ = ink;
}

static void puts_(const char *s)
{
    while (*s)
        putch(*s++);
}

static void put_at(unsigned char row, unsigned char x, unsigned char c, const char *s)
{
    at(row, x);
    ink = c;
    puts_(s);
}

static void put_num(unsigned char v, unsigned char digits)
{
    if (digits > 2)
        putch((char)('0' + v / 100));
    putch((char)('0' + (v / 10) % 10));
    putch((char)('0' + v % 10));
}

static unsigned char suit_ink(unsigned char color)
{
    switch (color) {
    case COLOR_RED: return C_RED;
    case COLOR_YELLOW: return C_YELLOW;
    case COLOR_GREEN: return C_GREEN;
    case COLOR_BLUE: return C_BLUE;
    }
    return C_BLACK;
}

static char suit_letter(unsigned char color)
{
    switch (color) {
    case COLOR_RED: return 'R';
    case COLOR_YELLOW: return 'Y';
    case COLOR_GREEN: return 'G';
    case COLOR_BLUE: return 'B';
    }
    return '?';
}

static char value_char(unsigned char value)
{
    switch (value) {
    case VAL_SKIP: return 'S';
    case VAL_REVERSE: return 'V';
    case VAL_DRAW2: return 'D';
    case VAL_WILD: return 'W';
    case VAL_WILD4: return 'F';
    }
    return (char)('0' + value);
}

/* The quick-play key for a hand slot: 1-9, 0, then A-J. */
static char slot_label(unsigned char i)
{
    if (i < 9)
        return (char)('1' + i);
    if (i == 9)
        return '0';
    if (i < QUICK_KEYS)
        return (char)('A' + i - 10);
    return ' ';
}

static const char *const color_names[4] = { "red", "yellow", "green", "blue" };

static void clear_rows(void)
{
    unsigned char *s = SCRBUF + FIRST_ROW * COLS;
    unsigned char *c = COLBUF + FIRST_ROW * COLS;
    unsigned int n = (LAST_ROW - FIRST_ROW + 1) * COLS;
    while (n--) {
        *s++ = ' ';
        *c++ = C_BLACK;
    }
}

static void draw_title(void)
{
    put_at(3, 18, C_YELLOW, "uno");
    put_at(6, 8, C_LBLUE, "a real c64 os application");
    put_at(7, 12, C_LBLUE, "built by llvm-mos");
    put_at(10, 6, C_GREEN, "press space for a new game");
}

static void draw_opponents(void)
{
    static const unsigned char xs[3] = { 1, 14, 27 };
    unsigned char i;
    for (i = 0; i < 3; i++) {
        at(ROW_OPP, xs[i]);
        ink = C_BLACK;
        puts_("CPU");
        putch((char)('1' + i));
        putch(':');
        put_num(g.players[i + 1].count, 2);
        putch(' ');
        putch(g.current_player == i + 1 ? '<' : ' ');
    }
}

static void draw_table(void)
{
    unsigned char tc = g.top_card.color == COLOR_WILD ? g.top_color : g.top_card.color;

    put_at(ROW_TABLE, 1, C_BLACK, "draw:");
    put_num(g.draw_count, 3);

    put_at(ROW_TABLE, 15, C_BLACK, "top:[");
    ink = suit_ink(tc);
    putch(suit_letter(g.top_color));
    putch(value_char(g.top_card.value));
    ink = C_BLACK;
    putch(']');

    put_at(ROW_TABLE, 25, C_BLACK, "col:");
    ink = suit_ink(g.top_color);
    putch(suit_letter(g.top_color));

    put_at(ROW_TABLE, 33, C_BLACK, g.direction == 1 ? "dir ->" : "dir <-");
}

static void draw_status(void)
{
    if (thinking != NONE) {
        put_at(ROW_STATUS, 1, C_BLACK, "cpu");
        putch((char)('0' + thinking));
        puts_(" is thinking...");
    } else if (invalid_flash) {
        put_at(ROW_STATUS, 1, C_RED, "that card does not match!");
    } else if (g.current_player == 0 && state == ST_HAND) {
        put_at(ROW_STATUS, 1, C_BLACK, "your turn");
    }
}

static void draw_color_picker(void)
{
    unsigned char i;
    put_at(ROW_STATUS, 1, C_BLACK, "choose a color:");
    at(ROW_PROMPT, 1);
    for (i = 0; i < 4; i++) {
        ink = C_BLACK;
        putch(i == color_sel ? '[' : ' ');
        ink = suit_ink(i);
        puts_(color_names[i]);
        ink = C_BLACK;
        putch(i == color_sel ? ']' : ' ');
        putch(' ');
    }
}

static void draw_challenge(void)
{
    put_at(ROW_STATUS, 1, C_BLACK, "wild draw four played.");
    put_at(ROW_PROMPT, 1, C_BLACK, "challenge? ");
    putch(challenge_sel == 0 ? '[' : ' ');
    ink = C_GREEN;
    puts_("yes");
    ink = C_BLACK;
    putch(challenge_sel == 0 ? ']' : ' ');
    putch(' ');
    putch(challenge_sel == 1 ? '[' : ' ');
    ink = C_RED;
    puts_("no");
    ink = C_BLACK;
    putch(challenge_sel == 1 ? ']' : ' ');
}

static void draw_hand(void)
{
    Player *p = &g.players[0];
    unsigned char i;

    put_at(ROW_HAND_LABEL, 1, C_BLACK, "your hand:");
    put_num(p->count, 2);

    for (i = 0; i < p->count; i++) {
        Card c = p->hand[i];
        at((unsigned char)(ROW_HAND + i / CARDS_PER_ROW),
           (unsigned char)(1 + (i % CARDS_PER_ROW) * 6));
        ink = c.color == COLOR_WILD ? C_BLACK : suit_ink(c.color);
        rvs = (state == ST_HAND && i == cursor) ? 0x80 : 0;
        putch('[');
        putch(slot_label(i));
        putch(':');
        putch(c.color == COLOR_WILD ? '?' : suit_letter(c.color));
        putch(value_char(c.value));
        putch(']');
        rvs = 0;
    }
}

static void draw_keys(void)
{
    put_at(ROW_KEYS, 1, C_BLACK, "crsr l/r pick, space play, crsr up draw");
    put_at(ROW_KEYS + 1, 1, C_BLACK, "or play by key: 1-9, 0, A-J");
}

static void draw_game_over(void)
{
    if (g.winner == 0)
        put_at(8, 16, C_GREEN, "you win!");
    else
        put_at(8, 16, C_RED, "cpu wins.");
    put_at(12, 6, C_BLACK, "press space to play again");
}

static void render(void)
{
    clear_rows();
    rvs = 0;
    if (state == ST_TITLE) {
        draw_title();
        return;
    }
    if (state == ST_OVER) {
        draw_game_over();
        return;
    }
    draw_opponents();
    draw_table();
    if (state == ST_COLOR)
        draw_color_picker();
    else if (state == ST_CHALLENGE)
        draw_challenge();
    else
        draw_status();
    if (state != ST_COLOR && state != ST_CHALLENGE)
        put_at(ROW_PROMPT, 1, C_BLACK, event_line);
    put_at(ROW_EVENT, 1, C_BLACK, effect_line);
    draw_hand();
    draw_keys();
}

/* ---- event text ----------------------------------------------------- */

static char *lp;

static void l_begin(char *line)
{
    lp = line;
    *lp = 0;
}

static void l_str(char *line, const char *s)
{
    while (*s && lp < line + LINE_LEN)
        *lp++ = *s++;
    *lp = 0;
}

static void l_ch(char *line, char ch)
{
    if (lp < line + LINE_LEN)
        *lp++ = ch;
    *lp = 0;
}

static void l_who(char *line, unsigned char player)
{
    if (player == 0) {
        l_str(line, "you");
    } else {
        l_str(line, "cpu");
        l_ch(line, (char)('0' + player));
    }
}

static void l_card(char *line, Card c)
{
    l_ch(line, '[');
    l_ch(line, c.color == COLOR_WILD ? '?' : suit_letter(c.color));
    l_ch(line, value_char(c.value));
    l_ch(line, ']');
}

/* The game's event flags from the last action, as one line. */
static void describe_effects(void)
{
    l_begin(effect_line);
    if (g.flag_skip != NONE) {
        l_who(effect_line, g.flag_skip);
        l_str(effect_line, g.flag_skip == 0 ? " are skipped. " : " is skipped. ");
    }
    if (g.flag_reverse != NONE)
        l_str(effect_line, "reverse! ");
    if (g.flag_draw_player != NONE) {
        l_who(effect_line, g.flag_draw_player);
        l_str(effect_line, g.flag_draw_player == 0 ? " draw " : " draws ");
        l_ch(effect_line, (char)('0' + g.flag_draw_count));
        l_str(effect_line, ". ");
    }
    if (g.flag_uno_player != NONE) {
        l_str(effect_line, "uno! ");
        l_who(effect_line, g.flag_uno_player);
    }
}

/* ---- banking --------------------------------------------------------- */

/* C64 OS calls the draw callback with I/O banked out ($01 = $34) and the
 * key and menu handlers with it in ($36). Drawing must hit the colour
 * BUFFER, the RAM under I/O at $D800; sound needs the SID at $D400 and the
 * VIC's raster counter. Each switches for as long as it needs, then puts
 * $01 back exactly as it found it. */
#define CPU_PORT (*(volatile unsigned char *)0x01)

static unsigned char port_saved;

static void io_on(void)
{
    port_saved = CPU_PORT;
    if ((port_saved & 0x07) < 0x05) /* %101-%111 are the I/O-visible maps */
        CPU_PORT = (unsigned char)((port_saved & 0xF8) | 0x05);
}

static void io_off(void)
{
    CPU_PORT = port_saved;
}

/* ---- timing and input ----------------------------------------------- */

/* Waits n jiffies. Bounded, so a clock that is not running can only make
 * the pause short, never hang the app. */
static void pause(unsigned char n)
{
    unsigned int guard = 0;
    unsigned char last = JIFFY_LO;
    while (n) {
        if (JIFFY_LO != last) {
            last = JIFFY_LO;
            n--;
        }
        if (++guard == 0)
            break;
    }
}

/* Shows the board right now. markredraw only schedules a redraw for the
 * OS's next event-loop pass, which never comes while a chain of CPU turns
 * is still running inside one key event. Rendering alone is not enough:
 * render() fills C64 OS's buffers, and only its end-of-loop copy puts them
 * on the screen the VIC shows -- so this renders the way the draw callback
 * does (I/O out) and then does that copy itself. */
static void show(void)
{
    unsigned char p = CPU_PORT;
    CPU_PORT = (unsigned char)((p & 0xF8) | 0x04);
    render();
    CPU_PORT = p;
    os_present();
}

/* ---- sound ---------------------------------------------------------- */

/* What the last play caused, as the bare C64 port sounds it. */
static void effect_sounds(void)
{
    if (g.flag_win) {
        sfx_win();
        return;
    }
    if (g.flag_skip != NONE)
        sfx_skip();
    if (g.flag_reverse != NONE)
        sfx_reverse();
    if (g.flag_draw_player != NONE)
        sfx_draw_multi(g.flag_draw_count);
    if (g.flag_uno_player != NONE)
        sfx_uno();
}

static void sound_play(void)
{
    io_on();
    sfx_card_play();
    effect_sounds();
    io_off();
}

static void sound_draw(void)
{
    io_on();
    sfx_draw();
    io_off();
}

static void sound_invalid(void)
{
    io_on();
    sfx_invalid();
    io_off();
}

/* A challenge, taken or not, and what it cost. */
static void sound_challenge(unsigned char challenged, unsigned char was_legal)
{
    io_on();
    if (challenged) {
        if (was_legal)
            sfx_challenge_fail();
        else
            sfx_challenge_success();
    }
    effect_sounds();
    io_off();
}

/* Discards keys that arrived while the CPU was playing: they were pressed
 * against a board the player had not seen yet. */
static void flush_keys(void)
{
    while (!(os_readkprnt() >> 8))
        os_deqkprnt();
}

static void redraw(void)
{
    os_markredraw();
}

/* ---- turn flow ------------------------------------------------------ */

static void play_slot(unsigned char player, unsigned char idx, unsigned char chosen)
{
    Card c = g.players[player].hand[idx];
    l_begin(event_line);
    l_who(event_line, player);
    l_str(event_line, player == 0 ? " play " : " plays ");
    l_card(event_line, c);
    if (c.color == COLOR_WILD) {
        l_str(event_line, ", calls ");
        l_str(event_line, color_names[chosen]);
    }
    play_card(&g, idx, chosen);
    describe_effects();
    show();
    sound_play();
}

static void cpu_turn(void)
{
    unsigned char p = g.current_player;
    unsigned char idx, before;
    Card c;

    thinking = p;
    show();
    pause(PAUSE_THINK);
    thinking = NONE;

    idx = ai_choose_card(&g, p);
    if (idx != NONE) {
        c = g.players[p].hand[idx];
        play_slot(p, idx, c.color == COLOR_WILD ? ai_choose_color(&g, p) : 0);
    } else {
        before = g.players[p].count;
        c = draw_card(&g, p);
        /* An empty pile hands back a card that is not in the hand. */
        if (g.players[p].count != before && is_legal(&g, c)) {
            unsigned char chosen = c.color == COLOR_WILD ? ai_choose_color(&g, p) : 0;
            sound_draw();
            idx = (unsigned char)(g.players[p].count - 1);
            play_slot(p, idx, chosen);
            l_begin(event_line);
            l_who(event_line, p);
            l_str(event_line, " draws and plays ");
            l_card(event_line, c);
            if (c.color == COLOR_WILD) {
                l_str(event_line, ", calls ");
                l_str(event_line, color_names[chosen]);
            }
        } else {
            end_turn_no_play(&g);
            l_begin(event_line);
            l_who(event_line, p);
            l_str(event_line, " draws a card");
            l_begin(effect_line);
            show();
            sound_draw();
        }
    }
    show();
    pause(PAUSE_SHOW);
}

/* After any action: a win ends the game, a pending Wild Draw Four waits
 * on its victim, and CPU turns run until the human is up again. */
static void after_action(void)
{
    unsigned char ch, legal;
    for (;;) {
        if (g.flag_win) {
            state = ST_OVER;
            break;
        }
        if (g.wd4_pending) {
            if (g.wd4_victim == 0) {
                state = ST_CHALLENGE;
                challenge_sel = 0;
                break;
            }
            ch = ai_should_challenge_wd4(&g, g.wd4_victim);
            legal = g.wd4_was_legal;
            resolve_wd4(&g, ch);
            l_begin(event_line);
            l_who(event_line, g.wd4_victim);
            if (!ch)
                l_str(event_line, " does not challenge");
            else
                l_str(event_line, legal ? " challenges, and loses" : " challenges, and wins");
            describe_effects();
            state = ST_HAND;
            show();
            sound_challenge(ch, legal);
            pause(PAUSE_SHOW);
            continue;
        }
        if (g.current_player == 0) {
            state = ST_HAND;
            invalid_flash = 0;
            if (cursor >= g.players[0].count)
                cursor = 0;
            break;
        }
        state = ST_HAND;
        cpu_turn();
    }
    flush_keys();
    redraw();
}

static void start_new_game(void)
{
    srand(seed ^ ((unsigned int)JIFFY_MID << 8 | JIFFY_LO));
    game_new(&g);
    cursor = 0;
    invalid_flash = 0;
    thinking = NONE;
    l_begin(event_line);
    l_begin(effect_line);
    state = ST_HAND;
    after_action();
}

/* Plays the human's card in slot idx, via the colour picker for a wild. */
static void human_play(unsigned char idx)
{
    Card c = g.players[0].hand[idx];
    if (!is_legal(&g, c)) {
        invalid_flash = 1;
        show();
        sound_invalid();
        redraw();
        return;
    }
    invalid_flash = 0;
    if (c.color == COLOR_WILD) {
        pending_idx = idx;
        color_sel = 0;
        state = ST_COLOR;
        redraw();
        return;
    }
    play_slot(0, idx, 0);
    after_action();
}

static void human_draw(void)
{
    unsigned char before = g.players[0].count;
    Card c = draw_card(&g, 0);
    invalid_flash = 0;
    sound_draw();
    if (g.players[0].count != before && is_legal(&g, c)) {
        cursor = (unsigned char)(g.players[0].count - 1);
        l_begin(event_line);
        l_str(event_line, "you draw ");
        l_card(event_line, c);
        if (c.color == COLOR_WILD) {
            pending_idx = cursor;
            color_sel = 0;
            state = ST_COLOR;
            redraw();
            return;
        }
        play_slot(0, cursor, 0);
        l_begin(event_line);
        l_str(event_line, "you draw and play ");
        l_card(event_line, c);
        after_action();
        return;
    }
    end_turn_no_play(&g);
    l_begin(event_line);
    l_str(event_line, "you draw ");
    l_card(event_line, c);
    l_begin(effect_line);
    after_action();
}

/* 1-9, 0, then A-J (either case) -> hand slot, or NONE. */
static unsigned char quick_slot(unsigned char k)
{
    if (k == '0')
        return 9;
    if (k >= '1' && k <= '9')
        return (unsigned char)(k - '1');
    k &= 0x7F; /* shifted letters arrive with bit 7 set */
    if (k >= 0x41 && k <= 0x4A)
        return (unsigned char)(k - 0x41 + 10);
    if (k >= 0x61 && k <= 0x6A)
        return (unsigned char)(k - 0x61 + 10);
    return NONE;
}

static void handle_key(unsigned char k)
{
    unsigned char s;

    seed = seed * 31 + k + JIFFY_LO;

    switch (state) {
    case ST_TITLE:
    case ST_OVER:
        if (k == K_SPACE || k == K_RETURN)
            start_new_game();
        return;

    case ST_HAND:
        if (k == K_LEFT) {
            if (cursor)
                cursor--;
            invalid_flash = 0;
            redraw();
        } else if (k == K_RIGHT) {
            if (cursor + 1 < g.players[0].count)
                cursor++;
            invalid_flash = 0;
            redraw();
        } else if (k == K_UP) {
            human_draw();
        } else if (k == K_SPACE || k == K_RETURN) {
            human_play(cursor);
        } else {
            s = quick_slot(k);
            if (s != NONE && s < g.players[0].count) {
                cursor = s;
                human_play(s);
            }
        }
        return;

    case ST_COLOR:
        if (k == K_LEFT) {
            color_sel = (unsigned char)((color_sel + 3) & 3);
            redraw();
        } else if (k == K_RIGHT) {
            color_sel = (unsigned char)((color_sel + 1) & 3);
            redraw();
        } else if (k == K_SPACE || k == K_RETURN) {
            state = ST_HAND;
            play_slot(0, pending_idx, color_sel);
            after_action();
        }
        return;

    case ST_CHALLENGE:
        if (k == K_LEFT || k == K_RIGHT) {
            challenge_sel ^= 1;
            redraw();
        } else if (k == K_SPACE || k == K_RETURN) {
            unsigned char legal = g.wd4_was_legal;
            unsigned char ch = (unsigned char)(challenge_sel == 0);
            resolve_wd4(&g, ch);
            l_begin(event_line);
            if (!ch)
                l_str(event_line, "you do not challenge");
            else
                l_str(event_line, legal ? "you challenge, and lose" : "you challenge, and win");
            describe_effects();
            state = ST_HAND;
            show();
            sound_challenge(ch, legal);
            after_action();
        }
        return;
    }
}

/* ---- entry points, called through app.s ---------------------------- */

void uno_start(void)
{
    state = ST_TITLE;
    io_on();
    sid_init();
    io_off();
    /* Anything typed while the OS was loading us is not meant for us. */
    flush_keys();
}

void uno_draw(void)
{
    render();
}

/* One keyboard event. Drain the whole queue, handling each key against
 * the state it finds: a key that sits queued behind a long redraw would
 * otherwise be replayed against whatever state the NEXT key found. The
 * key delivered with the event is used only if the queue was empty. */
void uno_key(unsigned char event_key)
{
    unsigned int r;
    unsigned char any = 0;
    for (;;) {
        r = os_readkprnt();
        if (r >> 8)
            break;
        os_deqkprnt();
        any = 1;
        handle_key((unsigned char)r);
    }
    if (!any)
        handle_key(event_key);
}

unsigned int uno_msgcmd(unsigned char msg, unsigned char code)
{
    if (msg == MC_MENQ)
        return MSG_UNHANDLED | 0; /* enabled, not selected -- as main.s */
    if (msg == MC_MNU) {
        if (code == ACT_NEW) {
            start_new_game();
            return MSG_HANDLED(0);
        }
        if (code == ACT_QUIT) {
            os_quitapp();
            return MSG_HANDLED(0);
        }
    }
    return MSG_UNHANDLED;
}
