# UNO for the MEGA65

Written in C against [cc65](https://cc65.github.io/) (the `c64` target),
using [mega65-libc](https://github.com/mega65/mega65-libc) for the MEGA65's
VIC-IV video chip.

## Requirements

`cl65` on your `PATH` (from cc65), and — for `make run` — the
[Xemu](https://github.com/lgblgblgb/xemu) MEGA65 emulator (`xmega65`) plus a
MEGA65 ROM. `make` fetches and builds mega65-libc automatically on the first
build (it needs `git`); real hardware works too.

## Building and running

```sh
make                                   # build/uno.prg (clones+builds mega65-libc first time)
make run XMEGA65=/path/to/xmega65 M65ROM=/path/to/mega65-rom.bin
```

Or by hand:

```sh
xmega65 -rom /path/to/mega65-rom.bin -prg build/uno.prg
```

## What makes this port different

The MEGA65 is a modern recreation of the never-released Commodore 65. This
port compiles as a plain `c64`-target program (so it loads and runs in the
MEGA65's C64 mode) but brings up the **VIC-IV** video chip through
mega65-libc's `conioinit()`, giving a real per-cell colour + attribute text
screen. Cards render as colour-bordered boxes with the value glyph, the same
40-column style as the C128/CBM-510 ports, and sound uses the MEGA65's SID
(directly mapped at `$D400` in C64 mode — no bank-switching, unlike the
CBM-II port).

The shared card logic (`cards.c`/`game.c`/`ai.c`) is reused unchanged;
`mega65vid.c` (video), `mega65snd.c` (SID), `input.c` (keyboard + joystick)
and the 40-column `ui.c` sit on top.

Input is joystick (port 2) or keyboard — cursor left/right to pick a card,
space/return to play, cursor-up to draw, or `1`-`9`/`0`/`A`-`J` to jump
straight to a card.

## Notes from bringing it up

- **Direct VIC-IV via mega65-libc.** `mega65_init()` calls `conioinit()`
  (which unlocks the VIC-IV), enables the reverse attribute for solid colour
  swatches, and selects the upper/graphics font.
- **Screen codes, not ASCII.** mega65-libc's `cputcxy()` writes raw screen
  codes, so `mega65vid.c` translates PETSCII → screen code (so both text and
  the PETSCII box-drawing glyphs render). The UI's string literals are kept
  ASCII with `<ascii_charmap.h>` so that translation lines up.
- **Linking mega65-libc.** Its `random.o` defines `rand`/`srand`, which clash
  with cc65's stdlib (the shared game uses `rand()`), so the Makefile drops
  `random.o` from the built `libmega65.a`.
- **Frame timing.** `wait_vsync()` polls the C64-compatible VIC raster
  (`$D012`).

Verified end-to-end in Xemu's `xmega65` (`-headless -prgexit -screenshot`):
title screen, and the dealt table + hand rendering in colour card boxes.

## Possible follow-ups

The MEGA65 has more to offer that this first cut doesn't use yet:

- **80-column mode** (VIC-IV H640) for a roomier layout.
- **Legal-move dimming / a reprogrammed palette** — the VBXE and X16 ports
  gray out unplayable cards using custom dark-suit palette entries; doing the
  same here needs VIC-IV palette reprogramming brought up (the `$D100/$D200/
  $D300` writes weren't taking during bring-up).
- **Hardware-sprite card toss** (the MEGA65's enhanced sprites) and
  **stereo** using the second SID at `$D420`.
