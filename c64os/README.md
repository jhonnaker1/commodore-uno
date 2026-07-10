# UNO for C64 OS

A real windowed application for [C64 OS](https://c64os.com/) -- Greg Naçu's
commercial GUI operating system for the Commodore 64 -- not a shortcut that
just launches the bare-metal `../c64/` port from a C64 OS file browser.
`src/main.s` opens a real window with a native menu bar, draws through
C64 OS's own draw-context KERNAL calls, and reads keyboard events through
its own event-driven input API. It's written in 6502 assembly against
C64 OS's TMP-syntax SDK, since C64 OS predates (and isn't compatible with)
the cc65/C toolchain every other port in this repo uses.

Status: in progress. Boots into a real C64 OS window with a working
"Game" menu (New Game / Quit), real keyboard input (`kprntevt`, verified
moving an on-screen marker), and the full game engine (`src/engine.s`
-- deck build/shuffle, deal, turn advancement, Skip/Reverse/DrawTwo/Wild/
Wild Draw Four incl. challenge, CPU AI), hand-ported to 6502 assembly
from the shared `cards.c`/`game.c`/`ai.c` every other port in this repo
uses. Verified via a debug readout that a fresh deal gives 7 cards per
hand and the right draw pile count. Not wired up yet: the real card/hand/
table UI (currently just a debug number readout) and the actual
play/draw turn loop -- that's the next piece of work.

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
make run    # refreshes those files and boots C64 OS in x64sc with a 16MB REU
```

The **first time**, you need to install the app for real: with `make run`
booted into C64 OS (the app bundle files are mounted as a live host
filesystem on device 8), open C64 OS's File Manager, jump to the
Applications directory (**Go > Applications**, or F4), create a `uno`
folder there, and copy `about.t`/`main.o`/`menu.m` in from device 8 --
or use JiffyDOS `@` wedge commands from the plain BASIC prompt before
booting C64 OS:

```
@cd//os/applications
@md:uno
@c/uno/:main.o=8:main.o
@c/uno/:menu.m=8:menu.m
@c/uno/:about.t=8:about.t
```

See C64 OS's own
[Developer Environment guide](https://c64os.com/c64os/programmersguide/devenvironment)
for more on installing a hand-built app if this is your first one.

**After that**, `make run` refreshes the build and auto-types (via VICE's
`-keybuf`, i.e. simulated keystrokes at the plain BASIC prompt, before
C64 OS loads) a JiffyDOS copy of the fresh `main.o` on top of the
already-installed one, then boots straight into C64 OS -- no manual
copying needed for day-to-day iteration. You also don't need to restart
VICE between every change: since device 8 is a live host-filesystem
passthrough of `dist/uno_1.0/`, just run `make dist` to refresh the files
there, then re-copy `main.o` onto the installed app from within the
already-running C64 OS session (File Manager, or the `@c/uno/:main.o=8:main.o`
command from a BASIC/terminal prompt if you have one open).

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
  app's screen layer -- one for control keys (`kcmdevt`, reserved for
  CONTROL/COMMODORE+key combos and function keys per the Programmer's
  Guide) and one for printable keys (`kprntevt`). Cursor keys, RETURN,
  DELETE, HOME, and CLR all count as *printable* key events, not control
  ones, despite the intuitive naming -- so all game navigation lives in
  `kprntevt`, and `kcmdevt` is currently unused (`sec` stub). `kprntevt`
  reads via `readkprnt_`/`deqkprnt_` but falls back to whatever arrived
  in `.A` on entry if the queue's already empty, since the guide says
  the OS delivers it there directly but that isn't independently
  documented anywhere else. Verified working: cursor left/right move an
  on-screen test marker.
- CPU turn pacing doesn't use a busy-wait loop the way every cc65 port's
  `pause_frames()` does -- C64 OS's event loop watches for unresponsive
  apps and animates a "CPU busy" warning if it doesn't get called every
  couple seconds, so a real delay needs the KERNAL's timer module
  (`timeque_`, an app-supplied trigger routine + countdown struct) rather
  than blocking. Not wired up yet; CPU turns currently resolve instantly
  with no "thinking" pause.
- `src/engine.s` packs each card into a single byte (`color<<4|value`)
  rather than the two-byte struct every cc65 port uses, and stores all
  four players' hands as one contiguous 160-byte array (`player_hands`,
  indexed by `player*40+slot` via a small `mul40` helper) instead of
  four separate arrays, so a hand slot is one absolute-indexed access
  instead of needing zero-page pointer indirection per player. TMP
  doesn't support C-style `<<` (it collides with `</>` meaning low/high
  byte of an address), so packed-card constants are precomputed with
  plain multiplication instead. TMP also has no `.fill`/`.res`-style
  directive for reserving a block of bytes; array reservations are
  explicit `.byte 0,0,0...` lists (tried the "advance the location
  counter without emitting data" idiom first, assuming C64 OS's loader
  wouldn't care -- it turned out to make no difference either way once
  tested, so the explicit form was kept since it's the more obviously
  correct one). Also: `.block`/`.bend` scopes *all* labels declared
  inside it, including plain data bytes, not just code -- so any state
  shared between routines in different blocks has to live at file scope.
