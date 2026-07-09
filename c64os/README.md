# UNO for C64 OS

A real windowed application for [C64 OS](https://c64os.com/) -- Greg Naçu's
commercial GUI operating system for the Commodore 64 -- not a shortcut that
just launches the bare-metal `../c64/` port from a C64 OS file browser.
`src/main.s` opens a real window with a native menu bar, draws through
C64 OS's own draw-context KERNAL calls, and reads keyboard events through
its own event-driven input API. It's written in 6502 assembly against
C64 OS's TMP-syntax SDK, since C64 OS predates (and isn't compatible with)
the cc65/C toolchain every other port in this repo uses.

Status: skeleton only so far -- boots into a real C64 OS window with a
working "Game" menu (New Game / Quit) and draw-context text rendering, but
the UNO game engine itself hasn't been ported to assembly yet.

## Requirements

You need your own legally obtained copies of:

- `rom/c64os.dhd` -- your C64 OS hard drive image (from your C64 OS
  purchase/SD card). Tip from the upstream template this is based on: you
  may need to run C64 OS's own update procedure first if you're on an
  older DHD.
- `rom/cmd_hd_bootrom.bin` -- the [CMD HD Boot ROM](https://store.go4retro.com/commodore/cmd-hdd-boot-rom-2-80-binary-image/),
  needed because VICE emulates the C64 OS hard drive as a CMD HD.
- `rom/JiffyDOS_C64.bin` -- optional but recommended (faster disk I/O);
  [available from Retro Innovations](https://store.go4retro.com/jiffydos-64-kernal-rom-overlay-image/).
  If you skip it, drop `-kernal $(JD_KERNAL_ROM)` from the Makefile's
  `C64EMU_ROMFLAGS` to use VICE's stock KERNAL instead.

None of these are included here -- they're all copyrighted/commercial.
Put them in `rom/` (gitignored).

You also need `x64sc` and `c1541` (from [VICE](https://vice-emu.sourceforge.io/),
at least v3.5) and Python 3.8+ on your `PATH`. The TMPx cross-assembler
itself is downloaded automatically by the Makefile on first build.

## Building

```sh
make        # assembles src/main.s -> dist/uno_1.0/ (main.o, menu.m, about.t)
make d64    # packages that into dist/uno_1.0.d64
make run    # boots C64 OS in x64sc with the app disk on drive 8
```

Once C64 OS has booted (drive 9, the CMD HD), open drive 8's file listing
and install/launch `UNO` as an application the same way you would any
other C64 OS app -- see C64 OS's own
[Developer Environment guide](https://c64os.com/c64os/programmersguide/devenvironment)
if you haven't installed a hand-built app before.

## Notes

- `os` (a symlink to `inc/os`) has to exist at the project root. TMPx's
  `.include "os/h/whatever.h"` directives resolve relative to the current
  working directory, not the `-I` search path passed on its command line --
  confirmed by trial and error after `-I inc` alone silently failed to
  find headers that definitely existed under `inc/os/`. The upstream
  [c64os-example-app](https://github.com/woodrowbarlow/c64os-example-app)
  this Makefile/pipeline is adapted from has the exact same symlink for
  the same reason.
- `inc/os` is a vendored (not submoduled) copy of
  [OpCoders-Inc/c64os-dev](https://github.com/OpCoders-Inc/c64os-dev)'s
  SDK headers, MIT licensed. `tools/` (PETSCII `about.t`/`menu.m`
  generators) is vendored from c64os-example-app's `c64util/`, also MIT.
- Keyboard input arrives as two separate per-keystroke callbacks on the
  app's screen layer -- one for control keys (cursor keys, return, etc.,
  `kcmdevt` in `main.s`) and one for printable keys (`kprntevt`) -- rather
  than cc65-style polling. Both are stubbed out for now (`sec` = "not
  handled"); wiring these up to move a card-selection cursor and play/draw
  cards is the next step in porting the actual game.
