#include "tsound.h"

#define TED_SND1_FREQ_LO (*(unsigned char *)0xFF0E)
#define TED_SND2_FREQ_LO (*(unsigned char *)0xFF0F)
#define TED_SND2_FREQ_HI (*(unsigned char *)0xFF10)
#define TED_SND_CTRL (*(unsigned char *)0xFF11)
#define TED_MISC (*(unsigned char *)0xFF12)
#define TED_RASTER_LO (*(unsigned char *)0xFF1D)

/* TED has no filter, no ADSR, no ring modulation, and (unlike SID) no
   documentation I can fully verify without hearing it -- the exact
   per-channel "enable" bits in snd_ctrl were a guess, and evidently the
   wrong one, since notes were starting but never stopping. Rather than
   guess again, silence is now forced through the master volume nibble
   (bits 0-3), which mutes the whole chip regardless of what any other
   bit does. Channel 1's frequency is a 10-bit period value split across
   its own low byte and 2 bits borrowed from the misc register (read-
   modify-write, touching only those 2 bits). Lower period value means a
   higher pitch, the opposite of SID's convention. Channel 2/noise is
   left alone entirely to keep this reliable. */

static void wait_frame(void) {
    while (TED_RASTER_LO != 0) {}
    while (TED_RASTER_LO == 0) {}
}

static void hold(unsigned char frames) {
    while (frames--) wait_frame();
}

static void set_period(unsigned int period) {
    TED_SND1_FREQ_LO = (unsigned char)(period & 0xFF);
    TED_MISC = (unsigned char)((TED_MISC & 0xFC) | ((period >> 8) & 0x03));
}

/* Volume 0 = silent; this is the only thing relied on to stop a note. */
static void note_on(unsigned int period) {
    set_period(period);
    TED_SND_CTRL = 0x1F;
}

static void note_off(void) {
    TED_SND_CTRL = 0x00;
}

void tsound_init(void) {
    note_off();
}

/* Bright short blip. */
void sfx_card_play(void) {
    note_on(180);
    hold(4);
    note_off();
}

/* Low thump. */
void sfx_draw(void) {
    note_on(900);
    hold(6);
    note_off();
}

/* Scales with how many cards are being forced in. */
void sfx_draw_multi(unsigned char count) {
    unsigned char n = (count > 4) ? 4 : count;
    unsigned char i;
    for (i = 0; i < n; i++) {
        note_on((unsigned int)(900 - i * 60));
        hold(4);
        note_off();
        hold(2);
    }
}

/* Two low descending tones for an illegal move. */
void sfx_invalid(void) {
    note_on(1400);
    hold(8);
    note_off();
    note_on(1800);
    hold(8);
    note_off();
}

/* Two quick bright notes. */
void sfx_uno(void) {
    unsigned char i;
    for (i = 0; i < 2; i++) {
        note_on(150);
        hold(6);
        note_off();
        hold(2);
    }
}

/* Quick descending sweep for a skipped turn. */
void sfx_skip(void) {
    unsigned char i;
    note_on(100);
    for (i = 0; i < 8; i++) {
        set_period((unsigned int)(100 + i * 60));
        wait_frame();
    }
    note_off();
}

/* Up-then-down bounce for a direction reversal. */
void sfx_reverse(void) {
    unsigned char i;
    note_on(600);
    for (i = 0; i < 5; i++) {
        set_period((unsigned int)(600 - i * 80));
        wait_frame();
    }
    for (i = 0; i < 5; i++) {
        set_period((unsigned int)(200 + i * 80));
        wait_frame();
    }
    note_off();
}

/* Short ascending sting: the challenge succeeded. */
void sfx_challenge_success(void) {
    static const unsigned int notes[3] = {300, 220, 150};
    unsigned char i;
    for (i = 0; i < 3; i++) {
        note_on(notes[i]);
        hold(5);
        note_off();
    }
}

/* Single low buzz: the challenge failed. */
void sfx_challenge_fail(void) {
    note_on(1600);
    hold(12);
    note_off();
}

/* Short arpeggio fanfare. */
void sfx_win(void) {
    static const unsigned int notes[6] = {500, 400, 300, 260, 220, 150};
    unsigned char i;
    for (i = 0; i < 6; i++) {
        note_on(notes[i]);
        hold(8);
        note_off();
    }
}
