/* Sound effects for the NeXT, through NeXTSTEP's C sound library.

   Every NeXT has 16-bit sound out, so each effect is a real sampled
   sound: at start-up its notes are written into a sound of their own, as
   16-bit stereo samples at 44100 Hz -- the sound hardware's own format,
   which needs no conversion on the way out (22050 Hz mono, which needs
   the DSP, never finished playing in Previous) -- and playing an effect is
   SNDStartPlaying(), which returns at once while the sound plays on, so
   the game never waits for it. Its preempt flag stops the effect still
   playing, so sounds stay tied to what is on screen.

   The C library rather than SoundKit's Sound class: it is all that is
   needed, and its preempt flag is exactly UNO's rule.
*/
#import <sound/sound.h>
#import <math.h>
#import <stdio.h>
#import "nextsound.h"

#define RATE 44100
#define AMP 6000 /* of 32767: square waves are loud */

typedef struct { unsigned char midi; unsigned short ms; } Note; /* midi 0 = rest */

enum { CARD, INVALID, DRAW, SKIP, REVERSE, UNO, WIN, CH_OK, CH_FAIL, DRAW1, NSFX = DRAW1 + 6 };

static SNDSoundStruct *sounds[NSFX];
static int tag;

static const Note t_card[] = {{84, 40}, {0, 0}};
static const Note t_invalid[] = {{43, 90}, {0, 30}, {40, 140}, {0, 0}};
static const Note t_draw[] = {{72, 35}, {0, 0}};
static const Note t_skip[] = {{76, 60}, {64, 80}, {0, 0}};
static const Note t_reverse[] = {{64, 50}, {71, 50}, {64, 70}, {0, 0}};
static const Note t_uno[] = {{72, 70}, {76, 70}, {79, 110}, {0, 0}};
static const Note t_win[] = {{72, 90}, {76, 90}, {79, 90}, {84, 160}, {0, 60}, {79, 90}, {84, 300}, {0, 0}};
static const Note t_ch_ok[] = {{74, 80}, {81, 140}, {0, 0}};
static const Note t_ch_fail[] = {{62, 90}, {55, 180}, {0, 0}};

static int tune_samples(const Note *t)
{
    int n = 0;
    for (; t->ms; t++) n += t->ms * RATE / 1000;
    return n;
}

/* Writes one note (or rest) of n frames -- left and right the same --
   faded in and out over 3 ms. */
static void write_note(short *out, int n, unsigned char midi)
{
    int i, fade = RATE * 3 / 1000;
    double period;
    if (!midi) {
        for (i = 0; i < 2 * n; i++) out[i] = 0;
        return;
    }
    period = RATE / (440.0 * pow(2.0, (midi - 69) / 12.0));
    for (i = 0; i < n; i++) {
        int v = (fmod(i, period) < period / 2) ? AMP : -AMP;
        if (i < fade) v = v * i / fade;
        else if (n - i < fade) v = v * (n - i) / fade;
        out[2 * i] = out[2 * i + 1] = (short)v;
    }
}

/* Writes a tune, repeated `times` times, into a new sound. */
static SNDSoundStruct *make_sound(const Note *t, int times)
{
    int per = tune_samples(t), total = per * times, k;
    SNDSoundStruct *s;
    short *p;
    int err = SNDAlloc(&s, total * 4, SND_FORMAT_LINEAR_16, RATE, 2, 4);
    if (err != SND_ERR_NONE) {
        fprintf(stderr, "UNO: no sound (%s)\n", SNDSoundError(err));
        return NULL;
    }
    p = (short *)((char *)s + s->dataLocation);
    for (k = 0; k < times; k++) {
        const Note *n;
        for (n = t; n->ms; n++) {
            int len = n->ms * RATE / 1000;
            write_note(p, len, n->midi);
            p += 2 * len;
        }
    }
    return s;
}

void snd_init(void)
{
    static const Note t_one[] = {{67, 45}, {0, 35}, {0, 0}};
    int i;
    sounds[CARD] = make_sound(t_card, 1);
    sounds[INVALID] = make_sound(t_invalid, 1);
    sounds[DRAW] = make_sound(t_draw, 1);
    sounds[SKIP] = make_sound(t_skip, 1);
    sounds[REVERSE] = make_sound(t_reverse, 1);
    sounds[UNO] = make_sound(t_uno, 1);
    sounds[WIN] = make_sound(t_win, 1);
    sounds[CH_OK] = make_sound(t_ch_ok, 1);
    sounds[CH_FAIL] = make_sound(t_ch_fail, 1);
    /* drawing 1 to 6 cards: that many blips */
    for (i = 1; i <= 6; i++) sounds[DRAW1 + i - 1] = make_sound(t_one, i);
}

static void play(int which)
{
    int err;
    if (!sounds[which]) return;
    /* a new tag each time; priority 1, preempting what is playing */
    err = SNDStartPlaying(sounds[which], ++tag, 1, 1, NULL, NULL);
    if (err != SND_ERR_NONE)
        fprintf(stderr, "UNO: can't play a sound (%s)\n", SNDSoundError(err));
}

void sfx_card_play(void) { play(CARD); }
void sfx_invalid(void) { play(INVALID); }
void sfx_draw(void) { play(DRAW); }
void sfx_draw_multi(unsigned char count)
{
    if (count < 1) count = 1;
    if (count > 6) count = 6;
    play(DRAW1 + count - 1);
}
void sfx_skip(void) { play(SKIP); }
void sfx_reverse(void) { play(REVERSE); }
void sfx_uno(void) { play(UNO); }
void sfx_win(void) { play(WIN); }
void sfx_challenge_success(void) { play(CH_OK); }
void sfx_challenge_fail(void) { play(CH_FAIL); }
