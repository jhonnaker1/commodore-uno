# UNO for the MSX2

1 human player vs. 3 CPU opponents, full rules (Skip, Reverse, Draw Two,
Wild, Wild Draw Four with challenge) — the same game logic as every other
port in this repo (`cards.c`, `game.c`, `ai.c` are shared verbatim), with an
MSX-specific video, sound and input layer underneath.

It ships as a **16K cartridge ROM**, and runs on any MSX2 — and unchanged on
MSX2+ and turbo R, which are supersets of it.

## Controls

| Key | Action |
|---|---|
| ← → | Move the hand cursor |
| `SPACE` or `RETURN` | Play the selected card, or confirm |
| `U` or ↑ | Draw a card |
| `1`-`9`, `0`, `A`-`J` | Jump straight to that hand slot and play it |

A joystick in port 1 works everywhere the cursor keys and space do. The BIOS
reports the cursor keys and a real joystick through the same call, so the
game never finds out which one you are using — a small luxury most machines
in this repo don't offer.

`SPACE` is the port's natural confirm, because `GTTRIG` treats the space bar
and the joystick trigger as the same button. `RETURN` is accepted too: at the
Wild Draw Four challenge prompt it is the key a player actually reaches for,
and a confirm key that does nothing there is indistinguishable from a hang.
That prompt spells out `SPACE/RET` on screen for the same reason — it is the
one modal prompt in the game with no other way to find out what to press.

## Why MSX2 and not MSX1

The MSX1's VDP is the TMS9918A, the same chip as the TI-99/4A port in this
repo, with the same awkward constraint: colour belongs to a *group of eight
character codes*, not to a screen position. That port works around it by
spending character codes to buy colours.

The MSX2's V9938 removes the constraint outright and adds two things this
game wants:

- **A programmable palette.** SCREEN 5 shows 16 colours chosen from 512, so
  the four suits are the actual UNO red/yellow/green/blue rather than the
  nearest of fifteen fixed entries. They are set in `gfx_init()` and that is
  the whole of it.
- **A hardware blitter.** The command engine fills a rectangle from a
  15-byte register block, so card bodies, borders and screen clears cost the
  Z80 almost nothing. This is what makes real pixel-art cards affordable at
  3.58MHz — it is the same reason the Amiga and Atari ST ports can afford
  them, except those get there with a 68000 and this gets there with a Z80
  and a helpful VDP.

Text is the one thing the blitter cannot help with, so glyphs are still
expanded and pushed a byte at a time by the CPU. They come from the
machine's own ROM font, though, via the `CGTBL` pointer at `0x0004` — so the
cartridge carries no font of its own and the game comes out in the same
typeface as the rest of the machine.

Sound is the AY-3-8910 PSG: three square-wave voices plus noise. Its tone
period works out to `111861/Hz`, which is the identical constant to the
TI-99/4A port's SN76489 — both chips hang off the same 3.579545MHz
colourburst crystal.

## Building

```sh
brew install sdcc
make          # builds build/uno.rom
make run      # boots it in openMSX
```

No toolchain to build from source, which makes this the least painful port
in the repo to get started on: SDCC is bottled in Homebrew, and the
cartridge is a plain 16K binary. `MACHINE` picks the openMSX machine
(default `Philips_NMS_8250`); `OPENMSX` points at the emulator.

`src/crt0.s` is the whole C runtime — SDCC ships crt0 files for CP/M and for
bare Z80 boards, but not for an MSX cartridge. It carries the 16-byte header
the BIOS scans slots for (`"AB"` at `0x4000`, then the INIT vector), parks
the stack at `HIMEM` so it lands clear of the BIOS work area and of a disk
interface's variables if one is fitted, zeroes `.bss` and copies initialised
globals out of ROM.

The game occupies 11,279 bytes of the 16,384 available, so unlike the
TI-99/4A port there is no bank switching: it is one flat ROM image.

## Three things that bite on this hardware

**The command engine and the CPU share the VRAM port.** `gfx_fill_rect()`
returns as soon as the blitter has been *started*, not when it finishes — a
full-screen clear keeps painting for tens of milliseconds afterwards, and
anything the CPU writes to VRAM meanwhile gets overwritten behind it. The
symptom is precise and misleading: the first line of text after a clear
vanishes and every later line is fine. `vdp_idle()` has to be waited on
before CPU VRAM access, not just before issuing the next command.

**`NX == 0` means "the whole width", not "nothing".** The high-speed
commands fill whole bytes, and one byte is two pixels, so a one-pixel-wide
rectangle truncates to zero bytes — which the V9938 reads as a full-width
fill. Asking naively for the vertical edge of a frame therefore paints a
band right across the screen. `gfx_fill_rect()` rounds x down and w up to
even to keep that from happening.

**R#15 belongs to the BIOS.** It selects which status register port `0x99`
reads back, and the BIOS interrupt handler reads S#0 every frame to
acknowledge the VDP interrupt. Reading the blitter's busy flag in S#2 and
leaving R#15 pointing there means the handler never acknowledges the
interrupt again and the machine wedges — keyboard and all. Select, read,
put it straight back, interrupts off throughout.

**Don't guard those with SDCC's `__critical`.** It compiles to the
save-and-restore form:

```
ld a,i / di / push af / ...body... / pop af / ret po / ei
```

`ld a,i` copies IFF2 into the P/V flag, and carries a documented Z80
erratum: if a maskable interrupt is accepted during that very instruction,
P/V comes back 0 no matter what IFF2 actually held. The restore then reads
"interrupts were already off", skips the `ei`, and they stay off for good —
`JIFFY` stops counting and every `wait_vsync()` after that spins forever.
The game freezes wherever it happens to be, most visibly parked on
"CPU*n* IS THINKING…" in the middle of a `pause_frames()`.

This is not a rare corner. Every VDP register write and every VRAM address
setup goes through one of these, so a screen redraw runs thousands, against
a VBLANK every 20ms; the first build survived about seventeen seconds.
openMSX emulates the erratum faithfully, which is the only reason it was
findable without real hardware.

Restoring the previous state buys nothing here — interrupts are on for the
whole life of the program, and none of this is reachable from an interrupt
handler — so `msxvdp.c` uses plain `di`/`ei` via inline asm instead. It is
both correct and shorter.

## A compiler bug worth knowing about

SDCC 4.6.0's z80 backend miscompiles a loop that walks a string while also
reading a global pointer variable. It allocates the string cursor to `IY`,
then emits `ld iy,#_font` to reach the global *in the same expression* —
clobbering the cursor, saving and restoring the clobbered value around the
call, and then incrementing that instead of the string pointer. The loop
never reaches the terminating NUL and walks off through memory; here it
painted all 27KB of the visible bitmap before anyone noticed.

`gfx_text()` avoids it by caching the global in a local and indexing the
string with an integer rather than a second pointer. Neither change is
cosmetic — both are load-bearing.

Separately, SDCC 4.6.0 fails outright with a `Unbalanced stack` internal
compiler error on the shape `resolve_pending_wd4()` originally had (an
if/else assigning two different calls to one variable, in a function that
then goes on to do a good deal more). No optimisation flag avoids it, since
it is in the core code generator rather than the peephole or the register
allocator; splitting the branch into its own function does, which is what
`decide_challenge()` in `main.c` is for.

## Running

```sh
make run
```

or by hand:

```sh
openmsx -machine Philips_NMS_8250 -carta build/uno.rom
```

The cartridge boots straight into the game — no BASIC, no menu, no keypress
needed to get past a selection screen. openMSX ships C-BIOS, which is enough
to run it if you have no real MSX2 system ROMs (`-machine C-BIOS_MSX2`).
