# UNO for the TI-99/4A

1 human player vs. 3 CPU opponents, full rules (Skip, Reverse, Draw Two,
Wild, Wild Draw Four with challenge) — the same game logic as every other
port in this repo (`cards.c`, `game.c`, `ai.c` are shared verbatim), with a
TI-specific video, sound and input layer underneath.

This is the only TMS9900 machine here, and the only 16-bit CPU besides the
68000 ports. It ships as a **16K bank-switched cartridge**.

## Controls

| Key | Action |
|---|---|
| `,` `.` | Move the hand cursor left / right |
| `SPACE` | Play the selected card, or confirm |
| `U` | Draw a card |
| `1`-`9`, `0`, `A`-`J` | Jump straight to that hand slot and play it |

The TI has no dedicated cursor keys — the arrows are `FCTN`+`S/D/E/X` and come
back as control codes rather than plain ASCII — so movement uses the same
comma/period scheme as the Amiga and F256 ports. Draw is `U` rather than the
more obvious `D` because `A`-`J` are already spoken for by the quick-play keys.

## Colour on a machine with no per-cell colour

The TMS9918A in Graphics I mode has no colour RAM and no per-cell attribute
byte. There is one 32-byte colour table for the whole screen, and each entry
colours a **group of eight consecutive character codes**. Colour belongs to
the character code, not the screen position: any two cells showing the same
code are necessarily the same colour, everywhere.

The classic TI answer, used here, is to spend character codes to buy colours —
keep several copies of the glyphs you want in colour, each copy in its own
colour group. This port carries six 24-code ranges (four suits, wild, and a
highlight range for the cursor), each holding the same small card alphabet, and
picks which copy to draw by colour. The alphabet itself is 23 glyphs — a range
runs to 24 because colour can only be assigned a whole 8-code group at a time,
so three groups is the smallest that fits, leaving one code spare.
The result is genuinely solid colour card
tiles, the same look as the X16/VBXE/F256 tile ports, on hardware with no
per-cell colour at all. The full budget — exactly 32 groups — is laid out in
[`src/tivid.h`](src/tivid.h).

The cost is that only those 23 glyphs exist in colour, which is why running
text is white and the colour picker shows `[R] [Y] [G] [B]` tiles rather than
the colour names in their own colours.

Sound is the SN76489 (TI's TMS9919): three real square-wave voices plus noise,
so UNO and the win flourish are three-voice chords rather than the single
clicks the 1-bit beeper ports (PET/Apple/Spectrum) manage.

## Why the cartridge is a loader

The game is about 12K of TMS9900 code, and a cartridge only shows 8K at
`>6000-7FFF`. So this is a two-bank `paged16k` cart, and rather than make the C
code bank-aware, [`src/loader.asm`](src/loader.asm) copies the whole program
out of both banks into the 32K expansion RAM at `>A000` and jumps there. After
that the ROM is never touched and the game runs as ordinary flat code, with the
stack at `>4000` in the *other* expansion block so it cannot grow into the code.

The one subtlety: that stub is byte-for-byte identical at the same address in
both banks, because switching banks swaps the memory out from under the program
counter — the instruction stream has to continue unchanged on the other side of
the switch. [`tools/pack.py`](tools/pack.py) writes the stub into both banks,
splits the payload across the leftover space, and patches the stub's parameter
block with the addresses and sizes the linker actually produced.

**The 32K expansion is required** — it's where the program runs.

## Building

The toolchain is gcc 4.4.0 + binutils 2.19.1 with
[mburkley's TMS9900 patches](https://github.com/mburkley/tms9900-gcc), built
from source into a per-user prefix (same idea as `CMOC_DIR`/`Z88DK_DIR` in the
CoCo and ZX Spectrum ports), plus
[Tursi's libti99](https://github.com/tursilion/libti99).

The upstream instructions are Debian-only, and on **Apple Silicon two fixes are
needed** — without the first, the compiler segfaults on every input, including
`int add(int a, int b) { return a + b; }`:

1. **`gcc/recog.h`** declares the pointer used to call every generated RTL
   pattern function as `typedef rtx (*insn_gen_fn) (rtx, ...);`. Those functions
   have fixed arity, so this only works where variadic and fixed arguments
   share a calling convention — true on x86-64 and on AAPCS64, but **not on
   Apple's arm64 ABI**, which passes variadic arguments on the stack while the
   callee reads registers. Every pattern is then built with garbage operands
   and cc1 dies in `mark_jump_label_1`. Change it to the unprototyped
   `typedef rtx (*insn_gen_fn) ();`, which gets the ordinary register-passing
   convention at whatever arity each call site uses.
2. **`gcc/config.host`** has no `aarch64*-*-darwin*` case, and the generic
   `*-darwin*` one pulls in `host-darwin.o`, which supplies only the PCH mmap
   helpers — the `host_hooks` struct lives in per-CPU files that have no
   aarch64 counterpart, so the link fails on undefined `_host_hooks`. Add an
   aarch64 case that falls through to the default `host-default.o`.

Also build both with `MAKEINFO=/usr/bin/true` (modern texinfo rejects the
2009 docs), and note that this backend has no working DImode support, so the
64-bit helpers in `config/tms9900/t-tms9900`'s `LIB2FUNCS_EXCLUDE` list have to
be extended — several existing entries there are typos (`_addvDI3`,
`__udivmoddi4`) that never matched anything.

Then:

```sh
make          # builds build/uno.rpk
make run      # launches it in MAME
```

`TI99_DIR` (default `~/ti99-toolchain`) and `LIBTI99_DIR` point at the
toolchain; `MAME`/`MAMEDIR` point at the emulator.

## Running

```sh
make run
```

MAME needs the peripheral expansion box with the 32K card plugged in, which
`make run` passes for you:

```sh
mame ti99_4a -cart build/uno.rpk -ioport peb -ioport:peb:slot2 32kmem \
  -skip_gameinfo -window -resolution 1024x768
```

At the TI title screen **press any key**, then **`2`** to pick UNO from the
console's selection list.

On real hardware, the `.rpk` is a zip — `uno_b0.bin` and `uno_b1.bin` inside it
are the two 8K banks, in that order.
