/* MS-DOS UNO: turn loop and game flow. The rules live in the shared
   cards.c/game.c/ai.c, identical to every other port; this drives them and
   draws through ui.c on top of the CGA text layer.

   Unlike the cartridge and disk ports, this one has somewhere to go when
   it finishes -- ESC returns to the DOS prompt, and cga_shutdown() puts the
   video mode, the cursor and the blink attribute back as it found them. */
#include <stdlib.h>
#include <dos.h>
#include "cgavid.h"
#include "pcspk.h"
#include "input.h"
#include "cards.h"
#include "game.h"
#include "ai.h"
#include "ui.h"

static GameState g;

static void pause_frames(unsigned int n)
{
    while (n--) wait_vsync();
}

static void pause_and_flush(unsigned int n)
{
    pause_frames(n);
    joy_flush();
}

static unsigned char human_has_legal_move(GameState *g)
{
    Player *p = &g->players[0];
    unsigned char i;
    for (i = 0; i < p->count; i++)
        if (is_legal(g, p->hand[i])) return 1;
    return 0;
}

static unsigned char handle_post_play_flags(GameState *g)
{
    if (g->flag_win) { sfx_win(); return 1; }
    if (g->flag_skip != NONE) { ui_event_skip(g->flag_skip); sfx_skip(); pause_frames(50); }
    if (g->flag_reverse != NONE) { ui_event_reverse(g->flag_reverse); sfx_reverse(); pause_frames(45); }
    if (g->flag_draw_player != NONE) {
        ui_event_draw(g->flag_draw_player, g->flag_draw_count);
        sfx_draw_multi(g->flag_draw_count);
        pause_frames(55);
    }
    if (g->flag_uno_player != NONE) { ui_event_uno(g->flag_uno_player); sfx_uno(); pause_frames(55); }
    return 0;
}

static unsigned char human_challenge_prompt(GameState *g)
{
    unsigned char sel = 0, pressed;
    joy_flush();
    ui_draw_challenge_prompt(g->wd4_victim, g->wd4_player, sel);
    for (;;) {
        wait_vsync();
        pressed = joy_pressed();
        if (joy_quit_requested()) return 0;
        if ((pressed & IN_LEFT) || (pressed & IN_RIGHT)) {
            sel = !sel;
            ui_draw_challenge_prompt(g->wd4_victim, g->wd4_player, sel);
        }
        if (pressed & IN_FIRE) return sel;
    }
}

static unsigned char resolve_pending_wd4(GameState *g)
{
    unsigned char challenged;

    if (g->wd4_victim == 0) challenged = human_challenge_prompt(g);
    else challenged = ai_should_challenge_wd4(g, g->wd4_victim);

    if (challenged) {
        unsigned char succeeded = !g->wd4_was_legal;
        resolve_wd4(g, challenged);
        ui_event_challenge_result(g->wd4_victim, g->wd4_player, succeeded);
        if (succeeded) sfx_challenge_success();
        else sfx_challenge_fail();
        ui_draw_opponents(g);
        ui_draw_hand(g, 0);
        pause_and_flush(60);
    } else {
        resolve_wd4(g, challenged);
    }
    ui_draw_opponents(g);
    ui_draw_table(g);
    ui_draw_hand(g, 0);
    return handle_post_play_flags(g);
}

static unsigned char after_play(GameState *g)
{
    if (g->wd4_pending) return resolve_pending_wd4(g);
    return handle_post_play_flags(g);
}

static unsigned char human_pick_color(GameState *g)
{
    unsigned char sel = 0, pressed;
    (void)g;
    joy_flush();
    ui_draw_color_picker(sel);
    for (;;) {
        wait_vsync();
        pressed = joy_pressed();
        if (joy_quit_requested()) return 0;
        if (pressed & IN_RIGHT) { sel = (unsigned char)((sel + 1) & 3); ui_draw_color_picker(sel); }
        if (pressed & IN_LEFT) { sel = (unsigned char)((sel + 3) & 3); ui_draw_color_picker(sel); }
        if (pressed & IN_FIRE) { ui_clear_color_picker(); return sel; }
    }
}

static unsigned char try_play(GameState *g, unsigned char idx, unsigned char *game_over)
{
    Card c = g->players[0].hand[idx];
    unsigned char chosen;

    if (!is_legal(g, c)) { sfx_invalid(); ui_event_invalid(); return 0; }
    chosen = (c.color == COLOR_WILD) ? human_pick_color(g) : 0;
    play_card(g, idx, chosen);
    sfx_card_play();
    ui_draw_opponents(g);
    ui_draw_table(g);
    ui_draw_hand(g, 0);
    *game_over = after_play(g);
    return 1;
}

static unsigned char human_turn(GameState *g)
{
    unsigned char cursor = 0, redraw = 1, pressed, qs, over, idx, chosen;
    Card drawn;

    for (;;) {
        if (redraw) {
            ui_draw_opponents(g);
            ui_draw_table(g);
            ui_draw_hand(g, cursor);
            if (human_has_legal_move(g))
                ui_message("YOUR TURN", "PICK A CARD AND PLAY IT");
            else
                ui_message("YOUR TURN", "NO PLAYABLE CARD - PRESS U TO DRAW");
            redraw = 0;
        }
        wait_vsync();
        pressed = joy_pressed();
        qs = joy_quick_select();
        if (joy_quit_requested()) return 1;

        if (pressed & IN_RIGHT) {
            if (cursor + 1 < g->players[0].count) cursor++;
            redraw = 1;
        }
        if (pressed & IN_LEFT) {
            if (cursor > 0) cursor--;
            redraw = 1;
        }
        /* Down/up move between the two rows of ten. */
        if (pressed & IN_DOWN) {
            if (cursor + HAND_PER_ROW < g->players[0].count) cursor = (unsigned char)(cursor + HAND_PER_ROW);
            redraw = 1;
        }
        if (pressed & IN_UP) {
            if (cursor >= HAND_PER_ROW) {
                cursor = (unsigned char)(cursor - HAND_PER_ROW);
                redraw = 1;
            } else {
                drawn = draw_card(g, 0);
                sfx_draw();
                ui_event_drew_one(0);
                ui_draw_opponents(g);
                ui_draw_hand(g, (unsigned char)(g->players[0].count - 1));
                pause_and_flush(30);
                if (is_legal(g, drawn)) {
                    idx = (unsigned char)(g->players[0].count - 1);
                    chosen = (drawn.color == COLOR_WILD) ? human_pick_color(g) : 0;
                    play_card(g, idx, chosen);
                    sfx_card_play();
                    ui_draw_opponents(g);
                    ui_draw_table(g);
                    ui_draw_hand(g, 0);
                    return after_play(g);
                }
                end_turn_no_play(g);
                return 0;
            }
        }
        if (qs != IN_NONE && qs < g->players[0].count) {
            cursor = qs;
            if (try_play(g, cursor, &over)) return over;
            redraw = 1;
        } else if (pressed & IN_FIRE) {
            if (try_play(g, cursor, &over)) return over;
            redraw = 1;
        }
    }
}

static unsigned char cpu_turn(GameState *g, unsigned char idx)
{
    unsigned char hand_idx, chosen;
    Card drawn, c;

    ui_draw_opponents(g);
    ui_draw_table(g);
    ui_draw_hand(g, 0);
    ui_event_thinking(idx);
    pause_frames(35);

    hand_idx = ai_choose_card(g, idx);
    if (hand_idx == NONE) {
        drawn = draw_card(g, idx);
        sfx_draw();
        ui_event_drew_one(idx);
        ui_draw_opponents(g);
        pause_frames(30);
        if (is_legal(g, drawn)) {
            hand_idx = (unsigned char)(g->players[idx].count - 1);
            chosen = (drawn.color == COLOR_WILD) ? ai_choose_color(g, idx) : 0;
            play_card(g, hand_idx, chosen);
            sfx_card_play();
        } else {
            end_turn_no_play(g);
            return 0;
        }
    } else {
        c = g->players[idx].hand[hand_idx];
        chosen = (c.color == COLOR_WILD) ? ai_choose_color(g, idx) : 0;
        play_card(g, hand_idx, chosen);
        sfx_card_play();
    }
    ui_draw_opponents(g);
    ui_draw_table(g);
    return after_play(g);
}

/* Unlike the cartridge ports, DOS has a real clock: the BIOS keeps a tick
   count at 0040:006C that has been running since boot, so the seed does not
   have to be scavenged from the player's reaction time. Mixed with a frame
   count anyway, since a machine booted straight into the game would
   otherwise start from a similar tick every time. */
static unsigned int seed_from_wait(void)
{
    unsigned long far *bios_ticks = (unsigned long far *)MK_FP(0x40, 0x6C);
    unsigned int counter = 0;

    for (;;) {
        wait_vsync();
        counter++;
        if (joy_pressed() & IN_FIRE) break;
        if (joy_quit_requested()) break;
    }
    return (unsigned int)(*bios_ticks) ^ counter;
}

int main(void)
{
    unsigned char game_over, human_won;

    cga_init();
    snd_init();

    while (!joy_quit_requested()) {
        ui_title_screen();
        joy_flush();
        srand(seed_from_wait());
        if (joy_quit_requested()) break;

        game_new(&g);
        game_over = 0;
        ui_draw_frame();
        ui_draw_opponents(&g);
        ui_draw_table(&g);
        ui_draw_hand(&g, 0);

        while (!game_over) {
            if (g.current_player == 0) game_over = human_turn(&g);
            else game_over = cpu_turn(&g, g.current_player);
            if (joy_quit_requested()) break;
        }
        if (joy_quit_requested()) break;

        human_won = (unsigned char)((g.winner == 0) ? 1 : 0);
        ui_game_over_screen(human_won, g.winner);
        joy_flush();
        for (;;) {
            wait_vsync();
            if (joy_pressed() & IN_FIRE) break;
            if (joy_quit_requested()) break;
        }
    }

    snd_shutdown();
    cga_shutdown();
    return 0;
}
