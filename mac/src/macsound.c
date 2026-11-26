/* Sound effects for the Mac, through the Sound Manager's square-wave
   synthesizer (noteSynth), which every Mac from the Plus has from System
   6.0 on. A channel is opened once at start-up; each effect queues a few
   notes on it and returns at once, so the tune plays while the game goes
   on -- the Toolbox equivalent of the other ports' fire-and-forget tone
   registers.

   Notes are MIDI numbers (60 = middle C) with durations in milliseconds.
   The shapes follow the Amiga port's: single blips for card/draw/skip/
   reverse, rising runs for UNO and winning, a falling pair for a failed
   challenge. If the Sound Manager won't give us a channel, every effect
   is silent except the "doesn't match" error, which falls back to the
   system beep. */
#include <Sound.h>
#include <OSUtils.h>
#include "macsound.h"

static SndChannelPtr chan;

#define AMP 0xC0 /* note amplitude, 0-255 */

static void cmd(short c, short p1, long p2) {
    SndCommand sc;
    sc.cmd = c;
    sc.param1 = p1;
    sc.param2 = p2;
    SndDoCommand(chan, &sc, false);
}

static void note(unsigned char midi, unsigned short ms) {
    cmd(noteCmd, (short)(ms * 2), ((long)AMP << 24) | midi); /* half-ms units */
}

static void rest(unsigned short ms) {
    cmd(restCmd, (short)(ms * 2), 0);
}

/* A new effect cuts off whatever is still playing rather than queueing
   behind it, so sounds stay tied to what's on screen. */
static int start(void) {
    SndCommand sc;
    if (!chan) return 0;
    sc.param1 = 0;
    sc.param2 = 0;
    sc.cmd = flushCmd;
    SndDoImmediate(chan, &sc);
    sc.cmd = quietCmd;
    SndDoImmediate(chan, &sc);
    return 1;
}

void snd_init(void) {
    if (SndNewChannel(&chan, noteSynth, 0, NULL) != noErr) chan = NULL;
}

void sfx_card_play(void) {
    if (!start()) return;
    note(84, 40);
}

void sfx_invalid(void) {
    if (!start()) { SysBeep(10); return; }
    note(43, 90);
    rest(30);
    note(40, 140);
}

void sfx_draw(void) {
    if (!start()) return;
    note(72, 35);
}

void sfx_draw_multi(unsigned char count) {
    unsigned char i;
    if (!start()) return;
    if (count > 6) count = 6;
    for (i = 0; i < count; i++) {
        note(67, 45);
        rest(35);
    }
}

void sfx_skip(void) {
    if (!start()) return;
    note(76, 60);
    note(64, 80);
}

void sfx_reverse(void) {
    if (!start()) return;
    note(64, 50);
    note(71, 50);
    note(64, 70);
}

void sfx_uno(void) {
    if (!start()) return;
    note(72, 70);
    note(76, 70);
    note(79, 110);
}

void sfx_win(void) {
    if (!start()) return;
    note(72, 90);
    note(76, 90);
    note(79, 90);
    note(84, 160);
    rest(60);
    note(79, 90);
    note(84, 300);
}

void sfx_challenge_success(void) {
    if (!start()) return;
    note(74, 80);
    note(81, 140);
}

void sfx_challenge_fail(void) {
    if (!start()) return;
    note(62, 90);
    note(55, 180);
}
