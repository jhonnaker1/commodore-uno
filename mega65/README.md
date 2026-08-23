# UNO for the MEGA65

The MEGA65 is a modern recreation of the never-released Commodore 65, and it
is really two machines: a C64 it can pretend to be, and the C65 it actually
is. This port is built twice, once for each, because no single binary reaches
both.

| Build | Toolchain | Load | Mode | Screen |
|---|---|---|---|---|
| `build/uno.prg` | cc65 (`c64` target) | `$0801` | C64 mode | 40x25, VIC-IV per-cell colour |
| `build/uno-native.prg` | llvm-mos (`mega65`) | `$2001` | C65/MEGA65 native | **80x25**, VIC-IV H640 |

The shared card logic (`cards.c`/`game.c`/`ai.c`) is identical in both, and
so is `mega65snd.c` -- the SID is at `$D400` either way.

## Requirements

For the C64-mode build: `cl65` on your `PATH` (from cc65). For the native
build: the [llvm-mos SDK](https://github.com/llvm-mos/llvm-mos-sdk/releases),
which ships **prebuilt for macOS, Linux and Windows** -- point `LLVM_MOS` at
it (default `~/llvm-mos`). Both builds fetch and build their own copy of
[mega65-libc](https://github.com/mega65/mega65-libc) on first use (it needs
`git`); the two copies are built by different paths and are not
interchangeable.

For `make run`, the [Xemu](https://github.com/lgblgblgb/xemu) MEGA65
emulator (`xmega65`) plus a MEGA65 ROM. Real hardware works too.

## Building and running

```sh
make                # build/uno.prg         C64 mode, 40 columns
make native         # build/uno-native.prg  native mode, 80 columns
make run            # the C64-mode build in Xemu
make run-native     # the native build in Xemu
```

Pass `XMEGA65=/path/to/xmega65 M65ROM=/path/to/mega65-rom.bin` if they are
not where the Makefile expects.

## Why 80 columns needs native mode

This is the interesting part of the port, and it is easy to get wrong -- the
first attempt chased it from the wrong direction.

Setting VIC-IV's H640 bit from the C64-mode build does not give you 80
columns. The blocker is not the video chip, which is perfectly capable; it is
that a `$0801` load address puts the machine in **C64 mode**, and the C64
never had 80 columns. `setscreensize(80, 25)` sets the bit and leaves you
exactly where you were.

Colour RAM makes it concrete. An 80x25 screen is 2000 cells, each wanting its
own colour byte, and a C64-mode program can only reach the 1K window at
`$D800`. There is nowhere to put the other 976 bytes.

Native mode solves both at once. A `$2001` load address with a BASIC 65
header is what actually selects it, and from there mega65-libc's conio drives
H640 properly -- including the VIC-III horizontal-positioning fix at `$D04C`
that the mode needs -- and reaches the real colour RAM at `$FF80000` through
the 45GS02's 32-bit addressing. Every one of the 2000 cells gets its colour.

Since cc65 has no MEGA65 target, that build uses **llvm-mos**. Unlike the
TMS9900, vbcc and Open Watcom toolchains elsewhere in this repo, it did not
have to be built from source -- the SDK ships prebuilt. It also generates
much tighter code: 11,316 bytes versus cc65's 26,042 for the same game.

## What native mode changes in the code

**Input gets simpler, not harder.** `$D610` is the MEGA65's ASCII key
register: reading it gives the ASCII code of the key waiting, writing pops
it. So `input_native.c` is a dozen lines of direct hardware access with no
translation layer, where the C64-mode `input.c` needs cc65's `<conio.h>`,
`<cbm.h>` and `<joystick.h>`. There is no `<ascii_charmap.h>` anywhere in the
native sources either -- that include exists in the cc65 build purely to stop
cc65 applying a PETSCII charmap to string literals, and llvm-mos has no such
concept. Joystick is still plain CIA1 at `$DC00`; the MEGA65 keeps both CIAs.

**Cards get wider, and better.** At 80 columns a card is 5x4 instead of 3x4,
which is enough to draw it as a solid block of the suit colour with the value
**knocked out** of it using the VIC-IV's reverse attribute -- the same look
the X16, VBXE, F256 and DOS ports build from per-cell foreground+background,
here done with one attribute bit. A full 20-card hand splits 11/9 across the
width rather than the 40-column build's 10/10.

**The sprite card toss did not come across.** The C64-mode build glides a
VIC-II hardware sprite from the played card to the discard pile, which works
because a probe confirmed mega65-libc leaves the C64 arrangement intact:
screen at `$0400`, VIC bank 0, sprite pointers at `$07F8`. Native mode
relocates screen RAM (the base comes from `$D060`-`$D063`) and H640 changes
sprite pointer handling -- which is why the MEGA65 has extended pointer
registers at `$D06C`-`$D06E` in the first place. Rather than guess, the
animation is left out of the native build; putting it back wants the same
probe-first approach that got it working in C64 mode.

**Uppercase only.** `setuppercase()` selects the upper/graphics charset,
where screen codes `$41`-`$5A` are graphics symbols rather than lowercase
letters. A lowercase letter in a UI string comes out as a piece of
box-drawing -- which is exactly what happened to a stray `"x"` in the draw
pile count before it was caught.

## Verifying it

The C64-mode build is checked with Xemu's `-prgexit -screenshot`, which saves
the framebuffer when the program returns to the `READY.` prompt. **That does
not work for the native build**: llvm-mos binaries do not reliably return to
BASIC on the MEGA65, so the exit event never fires and the emulator runs
forever.

Sending **SIGTERM** flushes `-screenshot` and `-dumpscreen` just the same --
and it works on a program spinning in its main loop, which `-prgexit` never
could. That removes the awkward part of the old recipe, where a verify build
had to be patched to draw a state and then `return`.

`smoke_native.c` uses it. Five screens, one per build:

```sh
mos-mega65-clang ... -DSMOKE_SCREEN=N ...
```

- `0` the dealt table
- `1` the wild colour picker
- `2` the picker cleared, since its frame is taller than an ordinary message
- `3` a forced 20-card hand, for the 11/9 split
- `4` an **autoplay soak**: the real engine with the AI in every seat,
  redrawing every screen each turn until someone wins. The four static
  screens only prove a layout; this is the one that would surface a crash or
  colour RAM being walked on after a few hundred redraws.

Xemu auto-detects C65 mode from the `$2001` load address, so `-prg` alone is
enough; `-prgmode 65` states it explicitly.

## Possible follow-ups

- **The card toss in native mode**, per the sprite note above.
- **80x50.** `setscreensize()` accepts it, and the layout already derives
  from `COLS`/`ROWS`.
- **Legal-move dimming / a reprogrammed palette.** The VBXE and X16 ports
  gray out unplayable cards using custom dark-suit palette entries. The
  VIC-IV palette writes at `$D100/$D200/$D300` did not take during C64-mode
  bring-up; native mode is the obvious place to retry, since C64 mode was
  the likely reason.
- **Stereo** using the second SID at `$D420`.
