# Commodore UNO

UNO for classic 8-bit machines (mostly Commodore, plus one Atari, one
Apple, and one 16/32-bit Amiga): 1 human player vs. 3 CPU opponents, full
rules (Skip, Reverse, Draw Two, Wild, Wild Draw Four with challenge). The
8-bit ports are written in C against the [cc65](https://cc65.github.io/)
6502 toolchain and tested in [VICE](https://vice-emu.sourceforge.io/)
(Commodore), [Atari800](https://atari800.github.io/) (Atari), and
[MAME](https://www.mamedev.org/) (Apple II). The Amiga port is a
completely different CPU architecture (68000, not 6502), built with
[bebbo's amiga-gcc](https://github.com/AmigaPorts/m68k-amigaos-gcc) and
tested in [FS-UAE](https://fs-uae.net/).

Each platform is its own self-contained subdirectory sharing the same card
game logic (`cards.c/h`, `game.c/h`, `ai.c/h`) with a platform-specific
video, sound, and input layer underneath, since the hardware capabilities
vary wildly across this lineup.

| Directory | Machine | Status |
|---|---|---|
| [`c64/`](c64) | Commodore 64 | Complete — custom charset, hardware sprites, SID sound (filter/ring-mod), full card animations |
| [`c128/`](c128) | Commodore 128 (40-column, or 80-column VDC) | Complete — the primary build reuses the C64's VIC-IIe path (keyboard input is scanned directly off CIA1 rather than via the KERNAL buffer); `make run-vdc` builds an alternate 80-column version driving the 8563/8568 VDC chip instead |
| [`plus4/`](plus4) | Commodore Plus/4 | Complete — TED video/sound, stock font |
| [`pet/`](pet) | Commodore PET 4032 | Complete — monochrome text UI, single-voice VIA beeper, keyboard only (no joystick port) |
| [`vic20/`](vic20) | Commodore VIC-20 (+memory expansion) | Complete — redirects the VIC's video matrix back to $1E00 to dodge a real rendering bug at the KERNAL's relocated $1000, color-coded suits (letter + color, since the VIC-20 has real per-cell color after all) |
| [`atari/`](atari) | Atari 800XL | Complete — standard ANTIC text mode (no per-cell color, so cards use color letters + reverse-video selection like the VIC-20/PET), 4-channel POKEY sound |
| [`apple/`](apple) | Apple IIe (enhanced) | Complete — 40x24 text mode (no per-cell color, reverse-video selection like the VIC-20/PET/Atari), 1-bit speaker bit-banged for tones; ships as a ProDOS `.SYSTEM` file, see [`apple/`](apple) for how to get it onto a bootable disk image |
| [`amiga/`](amiga) | Commodore Amiga (68000, Kickstart 2.0+) | Complete — a custom Intuition screen with a real 8-color palette (not the default Workbench screen's washed-out few shades) drawn through console.device with ANSI escape codes, 4-channel Paula sound with a generated sine-wave tone and volume envelope, keyboard input via IDCMP_VANILLAKEY (comma/period/'U' for movement instead of cursor keys — see Controls) |

## Building

Each platform directory is independent and builds with `make` (requires
`cc65`/`cl65` on your `PATH`):

```sh
cd c64 && make run     # builds build/uno.prg and launches it in x64sc
cd c128 && make run    # build/uno128.prg in x128 (40-column, VIC-IIe)
cd c128 && make run-vdc # build/uno128vdc.prg in x128 (80-column, VDC)
cd plus4 && make run   # build/uno4.prg in xplus4
cd pet && make run     # build/uno.prg in xpet
cd vic20 && make run   # build/uno20.prg in xvic -memory all
cd atari && make run XLXE_ROM=/path/to/your/atarixl.rom  # build/uno.xex in atari800
cd apple && make      # build/uno.system -- see apple/ for the ProDOS disk-image step
cd amiga && make      # build/uno -- needs m68k-amigaos-gcc on your PATH, see below
```

`make` alone just builds; `make clean` removes build artifacts.

The Apple II port doesn't have a `make run` target: cc65 doesn't produce a
runnable ProDOS floppy image directly, and there's no free/Homebrew tool
for building one, so `apple/tools/make_disk.py` injects the compiled
`build/uno.system` into a copy of your own ProDOS 2.4.3 (or similar) boot
disk image, replacing an existing sizeable file's data blocks in place:

```sh
python3 tools/make_disk.py <template.po> build/uno.system <existing_file_to_repurpose> <out.po>
```

`<existing_file_to_repurpose>` must be a sapling (multi-block) file already
on the template disk with at least 26 data blocks free. Then boot
`<out.po>` in MAME's `apple2ee` driver and run `-UNO` from BASIC.SYSTEM (or
select it from whatever boot menu your template disk provides).

The Amiga port needs a real 68000 cross-compiler, not cc65 — there's no
Homebrew package for it, so build
[m68k-amigaos-gcc](https://github.com/AmigaPorts/m68k-amigaos-gcc) from
source first (its README has exact macOS prerequisites/build commands;
took about 10-15 minutes on Apple Silicon) and put its `bin/` directory on
your `PATH`. The build itself is then a single `cl65`-style invocation
(see `amiga/Makefile`) producing a plain executable, `build/uno` — no
disk image needed. To test it in [FS-UAE](https://fs-uae.net/), mount a
plain host folder containing that binary as a hard drive (no ADF/HDF
creation required) and a Kickstart 2.0+ ROM:

```sh
fs-uae --amiga-model=A1200 --kickstart_file=/path/to/your/kickstart.rom \
    --hard_drive_0=/path/to/a/folder/containing/build/uno
```

Boot to the AmigaDOS shell and run `uno` directly.

The C64 and C128 versions use a custom character set (real chargen ROM
glyphs for codes 0-127, hand-drawn card-art glyphs above that). The
generated headers are checked in under `src/charset_data.h` /
`src/charset_codes.h`, so a fresh clone builds without needing the ROM.
Regenerating them (`make charset`) requires pointing `CHARGEN_PATH` in
`tools/gen_charset.py` at your own dumped C64 chargen ROM — the ROM itself
isn't included here.

## Controls

Common across every port: joystick (where the hardware has a port) or
keyboard — cursor left/right to pick a card, space/return to play or
confirm, cursor up to draw, or jump straight to a card with `1`-`9`, `0`,
`A`-`J`. The Amiga port is the one exception: its cursor keys didn't work
reliably through the console input path, so movement is comma/period
(left/right) and `U` (draw) instead — the same fallback the C128 needed
for its own unreachable dedicated cursor keys.

## Notes

- **VIC-20**: with a memory expansion installed, the KERNAL relocates the
  screen matrix to $1000 by default -- and the real VIC chip has a
  genuine, reproducible rendering bug there (large parts of a correct,
  in-memory screen just don't display), independent of RAM amount or
  KERNAL. `vic20io.c`'s `vic20_init()` works around it by reprogramming
  the VIC's video-matrix registers back to $1E00 (the stock/unexpanded
  default, which renders fine) instead. The custom linker config
  (`vic20-highmem.cfg`) deliberately leaves $1000-$1FFF free of code/data
  so that's safe to do, but its non-contiguous memory layout also means
  cc65's auto-generated "SYS nnnn" BASIC-stub address comes out wrong
  after linking; `tools/patch_sys_addr.py` fixes it up as a build step.

- **Amiga**: the hand row wraps after 9 cards per row (18 max), not the
  20 most other ports allow. console.device reports an 80-column screen
  for a suitably-sized window, but confirmed empirically (a column-marker
  test) that only about the first 60 columns are actually displayed —
  neither `SA_Overscan=OSCAN_MAX` nor an explicit `SA_DClip` rectangle
  widened that in testing (both made it narrower), so `ui.c` just designs
  around the ~60-column safe area instead of the nominal 80.
