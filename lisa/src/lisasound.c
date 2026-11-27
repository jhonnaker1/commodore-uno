/* Sound effects on the Lisa's speaker.

   The speaker hangs off the keyboard VIA ($FCDD81, the one the COPS talks
   through). This is the boot ROM's own beep routine ($FE0B06), done one
   note at a time:
   - port B bits 1-3 (made outputs in DDRB) set the volume, 0-7; the ROM
     beeps at 4;
   - the shift register runs free (ACR bits 2-4 = 100), shifting its byte
     out to the speaker over and over at the rate timer 2's low latch
     sets; the ROM loads $0F, so each pass of the byte is one cycle of a
     square wave;
   - clearing those ACR bits stops it.
   The VIA runs at 500 kHz, and each bit takes two timer 2 timeouts of
   N + 2 cycles, so the tone is 500000 / (16 * (N + 2)) Hz with $0F, or
   twice that with $33 (two cycles a byte), which higher notes use so N
   stays large enough to tune finely. Like the ROM, the period is a
   quarter longer when the I/O board's status byte ($FCC031, in the floppy
   controller's shared memory) says the VIA's clock is the faster one.

   The game never waits for a sound: an effect queues its notes and
   returns, and snd_tick(), called once a frame from wait_vsync(), moves
   on to the next note when the last one's frames run out. A new effect
   cuts off whatever is still playing, as on the Mac.

   Notes are MIDI numbers (60 = middle C) with lengths in milliseconds:
   the Mac port's tunes, an octave higher, since the lowest note this way
   (N = 255) is about 122 Hz. */
#include "lisasound.h"

typedef unsigned char u8;

#define VIA_ORB (*(volatile u8 *)0xFCDD81)
#define VIA_DDRB (*(volatile u8 *)0xFCDD85)
#define VIA_T2CL (*(volatile u8 *)0xFCDD91)
#define VIA_SR (*(volatile u8 *)0xFCDD95)
#define VIA_ACR (*(volatile u8 *)0xFCDD97)
#define IO_STATUS (*(volatile u8 *)0xFCC031)

#define VOLUME 4   /* 0-7, the ROM's */
#define OCTAVE 12  /* added to every note */

/* C8 to B8, in Hz. */
static const unsigned short top_octave[12] = {
    4186, 4435, 4699, 4978, 5274, 5588, 5920, 6272, 6645, 7040, 7459, 7902,
};

typedef struct { u8 n, pat, frames; } Note; /* n = timer 2 latch; pat 0 = rest */

#define QLEN 16
static Note queue[QLEN];
static u8 q_len, q_pos, left;
static u8 fast_via;

static void speaker_off(void) {
    VIA_ACR = (u8)(VIA_ACR & 0xE3);
}

static void speaker_on(const Note *nt) {
    speaker_off();
    VIA_ACR = (u8)(VIA_ACR | 0x10);
    VIA_T2CL = nt->n;
    VIA_SR = nt->pat;
}

void snd_init(void) {
    u8 st = IO_STATUS;
    fast_via = (u8)((st & 0x80) && !(st & 0x20));
    VIA_DDRB = (u8)(VIA_DDRB | 0x0E);
    VIA_ORB = (u8)((VIA_ORB & 0xF1) | (VOLUME << 1));
    speaker_off();
}

/* One frame is 1/60 s. */
static u8 ms_frames(unsigned short ms) {
    unsigned short f = (unsigned short)((ms * 3 + 25) / 50);
    return (u8)(f ? f : 1);
}

static void add(u8 n, u8 pat, unsigned short ms) {
    Note *nt;
    if (q_len >= QLEN) return;
    nt = &queue[q_len++];
    nt->n = n;
    nt->pat = pat;
    nt->frames = ms_frames(ms);
}

static void note(u8 midi, unsigned short ms) {
    u8 m = (u8)(midi + OCTAVE);
    u8 oct = (u8)(m / 12);                 /* 9 = the C8 octave */
    unsigned short hz = top_octave[m % 12];
    unsigned long cyc, n;
    u8 pat = 0x0F;
    if (oct > 9) oct = 9;
    /* N + 2 = 31250 / f, and f = hz >> (9 - oct) */
    cyc = (31250UL << (9 - oct)) / hz;
    if (cyc < 64) { cyc = (62500UL << (9 - oct)) / hz; pat = 0x33; }
    if (fast_via) cyc += cyc >> 2;
    n = cyc > 257 ? 255 : cyc < 3 ? 1 : cyc - 2;
    add((u8)n, pat, ms);
}

static void rest(unsigned short ms) {
    add(0, 0, ms);
}

static void start(void) {
    q_len = q_pos = left = 0;
    speaker_off();
}

void snd_tick(void) {
    if (left && --left) return;
    if (q_pos >= q_len) {
        if (q_len) { speaker_off(); q_len = q_pos = 0; }
        return;
    }
    if (queue[q_pos].pat) speaker_on(&queue[q_pos]);
    else speaker_off();
    left = queue[q_pos].frames;
    q_pos++;
}

/* Kick off the first note now rather than at the next frame. */
static void go(void) {
    snd_tick();
}

void sfx_card_play(void) {
    start();
    note(84, 40);
    go();
}

void sfx_invalid(void) {
    start();
    note(43, 90);
    rest(30);
    note(40, 140);
    go();
}

void sfx_draw(void) {
    start();
    note(72, 35);
    go();
}

void sfx_draw_multi(unsigned char count) {
    unsigned char i;
    start();
    if (count > 6) count = 6;
    for (i = 0; i < count; i++) {
        note(67, 45);
        rest(35);
    }
    go();
}

void sfx_skip(void) {
    start();
    note(76, 60);
    note(64, 80);
    go();
}

void sfx_reverse(void) {
    start();
    note(64, 50);
    note(71, 50);
    note(64, 70);
    go();
}

void sfx_uno(void) {
    start();
    note(72, 70);
    note(76, 70);
    note(79, 110);
    go();
}

void sfx_win(void) {
    start();
    note(72, 90);
    note(76, 90);
    note(79, 90);
    note(84, 160);
    rest(60);
    note(79, 90);
    note(84, 300);
    go();
}

void sfx_challenge_success(void) {
    start();
    note(74, 80);
    note(81, 140);
    go();
}

void sfx_challenge_fail(void) {
    start();
    note(62, 90);
    note(55, 180);
    go();
}
