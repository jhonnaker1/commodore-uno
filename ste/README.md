# UNO for the Atari ST / STE

Written in C against [vbcc](http://www.compilers.de/vbcc.html) (the
`m68k-atari` target), rendering pixel-art cards into a 320x200 16-colour
bitmap. Runs on an ST or an STE; the title screen tells you which it found.

## Requirements

The **vbcc m68k-atari cross-toolchain** and the
[Hatari](https://hatari.tuxfamily.org/) emulator (Homebrew: `brew install
hatari`) plus a TOS or EmuTOS ROM image. There is no Homebrew package for
the toolchain and the old `vincentriviere/m68k-atari-mint` tap is gone, so it
has to be built from source — see "Building the toolchain" below.

## Building and running

```sh
make                      # build/UNO.TOS
make run                  # ... and launch it in Hatari as an STE
make smoke                # build/SMOKE.TOS -- a static video/card-art test
make run-smoke
```

`make` takes `VBCC=/path/to/vbcc` and `run` takes `TOS_ROM=/path/to/tos.img`
if yours live elsewhere. `make run` copies the binary into an emulated
GEMDOS drive and autostarts it.

Controls are **keyboard**: left/right pick a card, space plays it, up draws,
and `1`-`9`/`0`/`A`-`J` jump straight to a hand slot.

## What makes this port different

Every other machine in this repo has a text mode. **The ST does not** — it is
a pure bitmap, and not a chunky one: the framebuffer is **four bitplanes
interleaved by word**. Each group of 8 bytes covers 16 pixels as four 16-bit
words, one per plane, so a pixel's colour index is assembled from the same
bit position across those four words. That shapes the whole video layer
(`src/stevid.c`):

- **Plotting is a 4-word read-modify-write**, so `ste_fill_rect()` is the
  fast path: it builds one bit mask per 16-pixel word group and applies it to
  all four planes, touching whole words instead of pixels. Card faces are
  drawn as nested fills (suit-coloured rect, white body inset by 3px) so
  almost all of the work runs at word speed.
- **One palette, two machines.** The STE's 4-bit-per-gun colour registers
  keep the extra bit in bit 3 of each nibble as the *least* significant bit —
  which a plain ST simply ignores, showing the nearest of its 512 colours. So
  the port writes STE-encoded palette words unconditionally: 4096 colours on
  an STE, a graceful fallback on an ST, and no second code path.
- **It carries its own font.** With no text mode, and the ROM font only
  reachable through Line-A or the VDI, the port embeds an 8x8 font generated
  by `tools/genfont.py` (authored as ASCII art there, not hex).
- **Machine detection** reads the `_MCH` cookie under `Supexec` to label the
  title screen ST vs STE.
- **Sound** is the YM2149 via XBIOS `Dosound()`, which plays a command
  sequence off the 50Hz VBL, so effects never block the game loop and no
  supervisor mode is needed (`src/stesound.c`).
- **Input** is the keyboard via BIOS console device 2, whose `Bconin()`
  packs the raw scancode in bits 16-23 — needed because the cursor keys have
  no ASCII code. Joystick is not wired up: the ST's sticks sit behind the
  IKBD, which only reports them after being switched out of mouse mode and
  serviced on an interrupt vector, and Hatari can map the cursor keys to a
  joystick anyway.

## The bug worth knowing about: `Card` alignment on a 68000

This one bit twice, and it never came from ST-specific code — it came from
the *shared* game logic, where it had been harmless for the entire life of
the project.

`Card` is two `unsigned char`s. That gives it an alignment requirement of
**1**, and vbcc's m68k backend indeed sets the minimum struct alignment to 1
(`malign[]` in `machines/m68k/machine.c`). But the compiler still copies a
two-byte struct with a single **`move.w`** — and a 68000 traps with an
Address Error on any word access to an odd address. So a `Card` that happens
to land on an odd address is a crash waiting to happen.

**First occurrence — inside `GameState`.** The original field order put one
`draw_count` byte between the two `Card` arrays:

```c
Card draw_pile[DECK_SIZE];    /* offset 0   */
unsigned char draw_count;     /* offset 216 */
Card discard_pile[DECK_SIZE]; /* offset 217 <-- odd */
```

Every access to the discard pile was a coin flip. Symptom: an instant crash
the moment a game started, while the static card-art test rendered
perfectly.

**Second occurrence — a `Card` local on the stack.** Fixing the struct got a
game running, but it still died after a couple of turns, with
`move.w (d16,A7),(d16,A7)` — a stack-to-stack copy, i.e. a local `Card`
being passed *by value* (`is_legal(g, drawn)`). The compiler had put that
local on an odd frame offset, which it is equally entitled to do.

Chasing individual placements was clearly a losing game, so the real fix is
to make the type itself 2-aligned: `src/cards.h` gives `Card` a trailing
`unsigned short _pad` member. A member with alignment 2 forces the whole
struct to alignment 2, so *every* Card — in an array, in a struct, on the
stack, passed by value — is even. Nothing ever reads `_pad`; Cards are
always built field by field, so no initializer had to change.

`src/game.h` also keeps the Card-holding members grouped at the front, which
is no longer strictly required but keeps the layout tidy.

The wider lesson: none of the 6502 ports could ever have surfaced this,
because the 6502 has no alignment rules at all. It took a machine that
actually enforces them.

## Building the toolchain

```sh
# sources
curl -O http://phoenix.owl.de/tags/vbcc0_9hP2.tar.gz
curl -O http://sun.hasenbraten.de/vasm/release/vasm.tar.gz
curl -O http://sun.hasenbraten.de/vlink/release/vlink.tar.gz
curl -O http://phoenix.owl.de/vbcc/2022-05-22/vbcc_target_m68k-atari.tar.gz
curl -O http://phoenix.owl.de/vbcc/2022-05-22/vbcc_unix_config.tar.gz

cd vasm && make CPU=m68k SYNTAX=mot && cd ..
cd vlink && make && cd ..
```

vbcc itself needs one trick. `make TARGET=m68k` runs `bin/dtgen`, which is
**interactive**: with `</dev/null` it hangs forever, and answering `y` to
everything produces a broken empty `dt.h` (you get `unknown type name
'zllong'`). Its defaults are already correct for a little-endian host
cross-compiling to big-endian m68k, so accept them all:

```sh
cd vbcc && mkdir -p bin
make TARGET=m68k bin/dtgen
yes '' | ./bin/dtgen machines/m68k/machine.dt objects/m68k/dt.h objects/m68k/dt.c
touch objects/m68k/dt.h objects/m68k/dt.c
make TARGET=m68k
```

Then assemble an install directory: `bin/` gets `vbccm68k`, `vc`,
`vasmm68k_mot`, `vlink`; `config/` gets the `tos` config from
`vbcc_unix_config`; and `targets/m68k-atari` comes from the target archive.
Point `VBCC` at it and put `$VBCC/bin` on `PATH` — `vc` invokes the other
tools by name.

Verified end-to-end in Hatari (STE, EmuTOS 1.3.0): title screen with machine
detection, dealt table + fanned hand, cursor movement, card play, CPU turns,
reverse/skip handling.
