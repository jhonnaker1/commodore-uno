#include "msxsnd.h"
#include "msxvdp.h"   /* wait_vsync -- effect lengths are counted in frames */

__sfr __at 0xA0 PSG_ADDR;
__sfr __at 0xA1 PSG_DATA;

/* The PSG is clocked at 1.7897725MHz and divides by 16, so a tone period is
   111861/Hz. That is the same constant as the TI-99/4A port's SN76489:
   both chips hang off the 3.579545MHz colourburst crystal.

   Kept as a macro so every call site folds to a constant at compile time --
   a runtime 32-bit divide would drag in SDCC's long-division helper for no
   reason, exactly as it did on the TMS9900. */
#define PSG_CLOCK 111861UL
#define HZ(f) ((unsigned int)(PSG_CLOCK / (f)))

/* Mixer (R#7). The low six bits are tone/noise enables, active low, but
   bits 6 and 7 are the PSG's I/O port directions and on an MSX those are
   not ours to choose: port A is the joystick/keyboard input and port B is
   an output. Clobbering them costs the machine its joystick reads, so
   every mixer value is built from this base. */
#define MIX_BASE  0x80          /* port A input, port B output */
#define MIX_TONES 0x38          /* all three noise channels off */
#define MIX_ALL_TONE (MIX_BASE | MIX_TONES)          /* 3 tones, no noise */
#define MIX_TONE_AND_NOISE (MIX_BASE | 0x30)         /* + noise on channel C */

static void psg(unsigned char reg, unsigned char val) {
    PSG_ADDR = reg;
    PSG_DATA = val;
}

static void tone(unsigned char ch, unsigned int period, unsigned char vol) {
    psg(ch << 1, (unsigned char)(period & 0xFF));
    psg((ch << 1) + 1, (unsigned char)((period >> 8) & 0x0F));
    psg(8 + ch, vol);
}

static void quiet(void) {
    psg(8, 0);
    psg(9, 0);
    psg(10, 0);
}

static void frames(unsigned char n) {
    while (n--) wait_vsync();
}

/* One voice, one pitch, one length. */
static void blip(unsigned int period, unsigned char vol, unsigned char len) {
    psg(7, MIX_ALL_TONE);
    tone(0, period, vol);
    frames(len);
    quiet();
}

void snd_init(void) {
    psg(7, MIX_ALL_TONE);
    quiet();
}

void sfx_card_play(void) {
    blip(HZ(900), 12, 2);
    blip(HZ(1400), 11, 3);
}

void sfx_invalid(void) {
    blip(HZ(150), 13, 4);
    blip(HZ(110), 13, 5);
}

/* Noise rather than a tone: a card coming off the pile is a rustle, and
   channel C's noise generator is sitting there unused the rest of the time. */
void sfx_draw(void) {
    psg(6, 20);                       /* noise period -- fairly bright */
    psg(7, MIX_TONE_AND_NOISE);
    psg(10, 10);
    frames(3);
    quiet();
    psg(7, MIX_ALL_TONE);
}

void sfx_draw_multi(unsigned char count) {
    while (count--) {
        sfx_draw();
        frames(2);
    }
}

void sfx_skip(void) {
    blip(HZ(700), 12, 3);
    blip(HZ(350), 12, 6);
}

void sfx_reverse(void) {
    unsigned char i;
    psg(7, MIX_ALL_TONE);
    for (i = 0; i < 8; i++) {
        tone(0, (unsigned int)(HZ(300) - (unsigned int)i * 26), 12);
        frames(1);
    }
    quiet();
}

/* A major triad, one note per voice -- the thing a single-voice machine
   cannot do. */
void sfx_uno(void) {
    psg(7, MIX_ALL_TONE);
    tone(0, HZ(523), 13);   /* C5 */
    tone(1, HZ(659), 12);   /* E5 */
    tone(2, HZ(784), 12);   /* G5 */
    frames(18);
    quiet();
}

void sfx_win(void) {
    /* Arpeggiate the triad in, then let all three ring together. */
    psg(7, MIX_ALL_TONE);
    tone(0, HZ(523), 13); frames(6);
    tone(1, HZ(659), 12); frames(6);
    tone(2, HZ(784), 12); frames(6);
    tone(0, HZ(1046), 13);
    frames(30);
    quiet();
}

void sfx_challenge_success(void) {
    psg(7, MIX_ALL_TONE);
    tone(0, HZ(660), 13);
    tone(1, HZ(880), 12);
    frames(10);
    tone(0, HZ(880), 13);
    tone(1, HZ(1170), 12);
    frames(12);
    quiet();
}

void sfx_challenge_fail(void) {
    psg(7, MIX_ALL_TONE);
    tone(0, HZ(300), 13);
    tone(1, HZ(220), 12);
    frames(10);
    tone(0, HZ(200), 13);
    tone(1, HZ(150), 12);
    frames(14);
    quiet();
}
