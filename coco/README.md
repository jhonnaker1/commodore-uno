# UNO for the Tandy Color Computer

Written in C against [CMOC](https://github.com/stahta01/cmoc) (targets
the CoCo 3's 6809 CPU), compiled for the Disk Basic environment and run
under [XRoar](https://www.6809.org.uk/xroar/). A genuinely different
toolchain from every cc65 (6502) port in this repo, same reason as the
ZX Spectrum port's z88dk/Z80.

## Requirements

CMOC isn't brew-installable -- it has to be built from source:

```sh
git clone https://github.com/stahta01/cmoc /tmp/cmoc_build
cd /tmp/cmoc_build
./configure --prefix="$HOME/cmoc"
make
make install
```

`$HOME/cmoc` is the default the Makefile expects (override with
`CMOC_DIR=/path/to/cmoc`), same idea as `Z88DK_DIR` in the zxspectrum/
port's Makefile.

You also need a CoCo 3 ROM -- a 32KB combined Color/Extended/Super
Extended BASIC image, commonly named `coco3.rom` in ROM sets. It's
copyrighted and not included here; copy your own to `rom/coco3.rom`.

## Building

```sh
make          # build/uno.bin
make run      # boots it in XRoar
```

`XRoar` must be on your `PATH` (`brew install xroar` or similar).
`make run` uses XRoar's `-run` flag, which attaches the `.bin` directly
and autotypes `LOADM`/`EXEC` for us -- no disk image needed.

## Notes

- CMOC's own preprocessing step shells out to the host's plain `cc -E`,
  and it doesn't ship a `stdlib.h` of its own (rand()/srand() are
  declared in its `cmoc.h` instead) -- so `#include <stdlib.h>` in the
  shared, platform-independent `cards.c` (verbatim across every port in
  this repo, so left untouched) falls through to this Mac's system
  `stdlib.h`. On current Xcode that drags in libc++'s C++ compatibility
  headers and fails outright (missing `__BYTE_ORDER__`, bare
  `__has_builtin`, etc. -- all unrelated to this program, just a
  mismatch between CMOC's minimal preprocessing environment and a modern
  macOS SDK). `src/compat/stdlib.h` shadows it via an earlier `-I` with
  just `#include <cmoc.h>`, which already declares everything actually
  needed.
- The installed `cmoc` binary has `/usr/local/share/cmoc` baked in as
  its default `PKGDATADIR` regardless of `./configure --prefix`, unless
  rebuilt -- the Makefile exports `PKGDATADIR` at build time instead of
  requiring a rebuild if `CMOC_DIR` is customized.
- CoCo 3's `attr(fg, bg, blink, underline)` (Super Extended Color Basic,
  from `coco.h`) sets true per-character colour, unlike the PET/VIC-20/
  Atari/ZX Spectrum ports, which fake colour with a bracketed
  `[label:COLORLETTER+VALUE]` scheme because their hardware can't do
  per-character colour cheaply. This port keeps that same bracket/letter
  convention anyway for consistency with the rest of the project --
  `attr()` is only used for the selection highlight, not true card
  colours (a nice follow-up if ever wanted).
- `attr()`'s colour indices (0-7) are raw GIME text-palette slots, not
  RGB values, and this never touches the boot palette. Checked
  empirically in XRoar: index 1 as a foreground on the default
  background (index 0, renders green) turned out completely invisible
  -- same hue as the background. Index 4 (renders black) and index 7
  (renders orange) both read cleanly against green, so normal hand text
  uses 4 and the selection highlight uses 7, rather than the fg/bg swap
  the other ports' "inverse" flag uses -- using index 4 as a
  *background* gave inconsistent results in the same test, so the
  background just stays fixed at 0.
- No cc65-style pacing concern here: the compiled program runs as a
  normal Disk Basic `EXEC`'d machine-language routine, so Basic's own
  60Hz clock interrupt (incrementing the word at `$0112`, exposed as
  `getTimer()`) keeps ticking in the background the whole time.
  `wait_vsync()` just busy-waits for that word to change -- the same
  jiffy-clock trick every cc65 (6502) port in this repo already uses,
  and simpler than the ZX Spectrum port's workaround (which couldn't
  safely enable interrupts at all).
- Keyboard-only input (like the PET port), scanned directly via
  `isKeyPressed()`'s keyboard-matrix probes -- a real joystick port
  exists on the CoCo, but the quick-play scheme (1-9,0,A-J) already
  needs a keyboard, so this keeps one input path instead of two.
