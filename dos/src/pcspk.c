#include <conio.h>
#include "pcspk.h"
#include "cgavid.h"   /* wait_vsync -- effect lengths are counted in frames */

/* 8253 PIT. Channel 2's output is wired to the speaker, gated by the low
   two bits of the keyboard controller's port 0x61: bit 0 enables the timer
   gate, bit 1 connects the timer output to the speaker. */
#define PIT_CH2   0x42
#define PIT_CMD   0x43
#define SPK_PORT  0x61

/* Channel 2, low byte then high byte, mode 3 (square wave), binary. */
#define PIT_SETUP 0xB6

/* 1193182 / Hz. Kept as a macro so every call site folds to a constant at
   compile time rather than dragging in a runtime long divide -- the same
   reason the TI-99/4A and MSX2 ports do it this way. */
#define PIT_CLOCK 1193182UL
#define HZ(f) ((unsigned int)(PIT_CLOCK / (f)))

static void tone_on(unsigned int divider)
{
    outp(PIT_CMD, PIT_SETUP);
    outp(PIT_CH2, (int)(divider & 0xFF));
    outp(PIT_CH2, (int)((divider >> 8) & 0xFF));
    outp(SPK_PORT, inp(SPK_PORT) | 0x03);
}

static void tone_off(void)
{
    outp(SPK_PORT, inp(SPK_PORT) & 0xFC);
}

static void frames(unsigned char n)
{
    while (n--) wait_vsync();
}

static void blip(unsigned int divider, unsigned char len)
{
    tone_on(divider);
    frames(len);
    tone_off();
}

void snd_init(void)
{
    tone_off();
}

void snd_shutdown(void)
{
    /* Leaving the gate open would hold a tone through the DOS prompt. */
    tone_off();
}

void sfx_card_play(void)
{
    blip(HZ(900), 2);
    blip(HZ(1400), 3);
}

void sfx_invalid(void)
{
    blip(HZ(160), 4);
    blip(HZ(110), 5);
}

/* One voice and no noise generator, so a card coming off the pile is a
   short high tick rather than the MSX2's rustle of filtered noise. */
void sfx_draw(void)
{
    blip(HZ(2200), 2);
}

void sfx_draw_multi(unsigned char count)
{
    while (count--) {
        sfx_draw();
        frames(2);
    }
}

void sfx_skip(void)
{
    blip(HZ(700), 3);
    blip(HZ(350), 6);
}

void sfx_reverse(void)
{
    unsigned char i;
    for (i = 0; i < 8; i++)
        blip((unsigned int)(HZ(300) - (unsigned int)i * 26), 1);
}

/* Arpeggiated, not chorded -- one voice is one voice. The MSX2 and TI-99
   ports play this as a real triad. */
void sfx_uno(void)
{
    blip(HZ(523), 5);
    blip(HZ(659), 5);
    blip(HZ(784), 8);
}

void sfx_win(void)
{
    blip(HZ(523), 5);
    blip(HZ(659), 5);
    blip(HZ(784), 5);
    blip(HZ(1046), 14);
}

void sfx_challenge_success(void)
{
    blip(HZ(660), 5);
    blip(HZ(880), 5);
    blip(HZ(1170), 8);
}

void sfx_challenge_fail(void)
{
    blip(HZ(300), 6);
    blip(HZ(220), 6);
    blip(HZ(150), 10);
}
