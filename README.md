# Commodore UNO

UNO for classic 8-bit machines (mostly Commodore, plus two Atari, one
Apple, one Sinclair, and two 16/32-bit 68000 machines): 1 human player vs.
3 CPU opponents, full rules (Skip, Reverse, Draw Two, Wild, Wild Draw Four
with challenge). Most of the 6502-based ports are written in C against the
[cc65](https://cc65.github.io/) toolchain and tested in
[VICE](https://vice-emu.sourceforge.io/) (Commodore),
[Atari800](https://atari800.github.io/) (Atari), and
[MAME](https://www.mamedev.org/) (Apple II). The Amiga port is a
completely different CPU architecture (68000, not 6502), built with
[bebbo's amiga-gcc](https://github.com/AmigaPorts/m68k-amigaos-gcc) and
tested in [FS-UAE](https://fs-uae.net/). The Atari ST/STE port is 68000 as
well but a different target entirely (GEMDOS/TOS, not AmigaOS), built with
[vbcc](http://www.compilers.de/vbcc.html) and tested in
[Hatari](https://hatari.tuxfamily.org/). The ZX Spectrum port is a
different CPU family again (Z80, not 6502), built with
[z88dk](https://github.com/z88dk/z88dk) and tested in MAME's `spectrum`
driver.

Each platform is its own self-contained subdirectory sharing the same card
game logic (`cards.c/h`, `game.c/h`, `ai.c/h`) with a platform-specific
video, sound, and input layer underneath, since the hardware capabilities
vary wildly across this lineup.

**Prebuilt binaries** for every platform are attached to the
[latest release (v1.0.4)](https://github.com/jhonnaker1/commodore-uno/releases/tag/v1.0.4)
— grab the one for your machine and run it in the matching emulator (per-file
instructions are in the release notes). To build from source instead, see
[Building](#building) below.

| Directory | Machine | Status |
|---|---|---|
| [`c64/`](c64) | Commodore 64 | Complete — custom charset, hardware sprites, SID sound (filter/ring-mod), full card animations |
| [`c128/`](c128) | Commodore 128 (40-column, or 80-column VDC) | Complete — the primary build reuses the C64's VIC-IIe path (keyboard input is scanned directly off CIA1 rather than via the KERNAL buffer); `make run-vdc` builds an alternate 80-column version driving the 8563/8568 VDC chip instead |
| [`plus4/`](plus4) | Commodore Plus/4 | Complete — TED video/sound, stock font |
| [`pet/`](pet) | Commodore PET 4032 (40-column, or 80-column 8032) | Complete — monochrome text UI, single-voice VIA beeper, keyboard only (no joystick port); `make run-8032` builds an alternate 80-column version for the PET 8032 (same hardware family, just double the columns) |
| [`vic20/`](vic20) | Commodore VIC-20 (+memory expansion) | Complete — redirects the VIC's video matrix back to $1E00 to dodge a real rendering bug at the KERNAL's relocated $1000, color-coded suits (letter + color, since the VIC-20 has real per-cell color after all) |
| [`atari/`](atari) | Atari 800XL (stock, or with a VBXE video board) | Complete — the standard build uses ANTIC text mode (no per-cell color, so cards use color letters + reverse-video selection like the VIC-20/PET), 4-channel POKEY sound; `make all-vbxe` builds an alternate version for an 800XL fitted with a [VBXE](https://vbxe.atari.org/) video board, using its real char+attribute 80-column text mode for solid per-suit colored card tiles plus toss/deal animations, a blinking cursor, and a win flourish — at C64-port feature parity (see [`atari/`](atari)) |
| [`apple/`](apple) | Apple IIe (enhanced) | Complete — 40x24 text mode (no per-cell color, reverse-video selection like the VIC-20/PET/Atari), 1-bit speaker bit-banged for tones; ships as a ProDOS `.SYSTEM` file, see [`apple/`](apple) for how to get it onto a bootable disk image |
| [`amiga/`](amiga) | Commodore Amiga (68000, Kickstart 2.0+) | Complete — a custom Intuition screen with a real 8-color palette (not the default Workbench screen's washed-out few shades) drawn through console.device with ANSI escape codes, 4-channel Paula sound with a generated sine-wave tone and volume envelope, keyboard input via IDCMP_VANILLAKEY (comma/period/'U' for movement instead of cursor keys — see Controls). Also ships a separate **bitmap-graphics** build (`make bmp`) that renders the whole game as pixel-art cards in a 320×256 16-colour screen, with the card bodies drawn as blitter `RectFill`s and text via the topaz ROM font — the same look as the Atari ST and X16/VBXE bitmap builds (see [`amiga/`](amiga)) |
| [`cbm510/`](cbm510) | Commodore CBM-II (510/P500) | Complete — the one CBM-II model with a real VIC-II and SID (same chips as the C64, reached through cc65's `pokebsys()`/`peekbsys()` since they live in a separate bank-switched "system bank" plain pointers can't reach), full-color card borders and SID sound effects, same box-drawing charset as the C64/C128 (stock PETSCII, no custom chargen needed) |
| [`c64os/`](c64os) | Commodore 64, running [C64 OS](https://c64os.com/) | In progress — a real windowed C64 OS application (not a shortcut to the bare-metal `c64/` port), written in 6502 assembly against C64 OS's own TMP-syntax KERNAL, hand-assembled with the cross-platform TMPx assembler since it's a closed-source commercial OS with its own SDK, no C toolchain |
| [`zxspectrum/`](zxspectrum) | Sinclair ZX Spectrum 48K | Complete — Z80 (not 6502) via z88dk; 32x24 text over a bitmap with per-8x8-cell ink/paper color (the classic "attribute clash"), cards shown as bracketed color-letter labels like the VIC-20/PET/Atari ports, `O`/`P`/`Q` "keys as joystick" scheme since a real Spectrum has no cursor keys, 1-bit beeper sound |
| [`x16/`](x16) | Commander X16 | Complete — the modern 8-bit machine; talks straight to its VERA video chip (per-cell fg+bg color) for solid colored card tiles with legal-move dimming and a pulsing selection highlight, a real **hardware-sprite** card toss (VERA has 128 sprites), and VERA PSG sound — the same feature set as the C64/VBXE ports. Also ships a separate **bitmap-graphics** build (`make bmp`) that renders the whole game in a 320×240 256-color framebuffer via the KERNAL GRAPH API — pixel-drawn card faces and a fanned hand |
| [`ste/`](ste) | Atari ST / STE (68000) | Complete — the only machine here with **no text mode at all**, and its framebuffer is four bitplanes interleaved by word rather than chunky, so the video layer masks whole 16-pixel word groups across all four planes and draws card faces as nested fills to stay at word speed. Full-colour pixel-art cards and a fanned hand in 320×200/16 colours. Writes the palette in **STE 4-bit-per-gun encoding**, whose extra bit the ST simply ignores — 4096 colours on an STE, a graceful fallback to the ST's 512 from one code path — and detects which machine it's on via the `_MCH` cookie. Carries its own 8×8 font (generated by [`tools/genfont.py`](tools/genfont.py)) since the ROM font is only reachable through Line-A/VDI, YM2149 sound via XBIOS `Dosound()`, keyboard input. Built with vbcc, not cc65 — see [`ste/`](ste) |
| [`mega65/`](mega65) | MEGA65 | Complete — the modern Commodore-65 recreation; compiles as a `c64`-target program that brings up the VIC-IV video chip via [mega65-libc](https://github.com/mega65/mega65-libc) for a per-cell-color 40-column screen (color-bordered card boxes), a **VIC-II hardware-sprite** card toss on every play, and SID sound directly mapped at `$D400`. (Legal-move dimming and stereo dual-SID are noted follow-ups; 80-column needs native C65 mode — see [`mega65/`](mega65).) |

## Building

Each platform directory is independent and builds with `make` (requires
`cc65`/`cl65` on your `PATH`):

```sh
cd c64 && make run     # builds build/uno.prg and launches it in x64sc
cd c128 && make run    # build/uno128.prg in x128 (40-column, VIC-IIe)
cd c128 && make run-vdc # build/uno128vdc.prg in x128 (80-column, VDC)
cd plus4 && make run   # build/uno4.prg in xplus4
cd pet && make run     # build/uno.prg in xpet
cd pet && make run-8032 # build/uno8032.prg in xpet -model 8032
cd vic20 && make run   # build/uno20.prg in xvic -memory all
cd atari && make run XLXE_ROM=/path/to/your/atarixl.rom  # build/uno.xex in atari800
cd atari && make all-vbxe  # build/unovbxe.xex for an 800XL with a VBXE video board (see atari/ for running it)
cd apple && make      # build/uno.system -- see apple/ for the ProDOS disk-image step
cd amiga && make      # build/uno -- needs m68k-amigaos-gcc on your PATH, see below
cd amiga && make bmp  # build/unobmp -- the bitmap-graphics build (see amiga/)
cd cbm510 && make run # build/uno.prg in xcbm5x0
cd c64os && make run  # dist/uno_1.0.d64 in x64sc, booting C64 OS -- see c64os/ below
cd zxspectrum && make Z88DK_DIR=/path/to/z88dk run  # build/uno.sna in MAME's spectrum driver
cd x16 && make run X16EMU=/path/to/x16emu_dir  # build/uno.prg in the Commander X16 emulator
cd mega65 && make run XMEGA65=/path/to/xmega65 M65ROM=/path/to/mega65-rom.bin  # build/uno.prg in Xemu's xmega65
cd ste && make run VBCC=/path/to/vbcc TOS_ROM=/path/to/tos.img  # build/UNO.TOS in Hatari as an STE
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

The Atari ST/STE port is 68000 too, but the Amiga compiler can't build it —
it emits AmigaOS hunk executables against `exec.library`/`dos.library`,
while TOS needs GEMDOS `$601A` programs. It uses
[vbcc](http://www.compilers.de/vbcc.html) with the `m68k-atari` target
instead, which also has no Homebrew package (and the old
`vincentriviere/m68k-atari-mint` tap is gone), so it too is built from
source — [`ste/README.md`](ste/README.md) has the exact commands, including
the one non-obvious step: vbcc's `dtgen` configuration stage is interactive
and silently produces a broken compiler if answered wrong. Then:

```sh
cd ste && make VBCC=/path/to/vbcc      # build/UNO.TOS
```

`make run` copies it onto an emulated GEMDOS drive and autostarts it in
Hatari; you'll need a TOS or EmuTOS ROM image.

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
for its own unreachable dedicated cursor keys. The Atari ST/STE port is
keyboard-only (cursor keys and space): the ST's joystick ports sit behind
the IKBD, which reports sticks only after being switched out of mouse mode
and serviced on an interrupt vector, and Hatari can map the cursor keys to
a joystick anyway.

## Notes

- **Atari ST/STE — a struct-alignment bug the 6502 ports had been hiding.**
  The ST port crashed with an Address Error the instant a game started,
  while its static card-art test rendered perfectly. The fault wasn't in
  any ST code: the *shared* `GameState` put a single `draw_count` byte
  between the two `Card` arrays, so `discard_pile` began on odd offset 217.
  `Card` is two bytes with byte alignment, so that's legal — but the
  compiler renders a two-byte `Card` assignment as one `move.w`, and a
  68000 traps on a word access to an odd address. Completely harmless on
  every 6502 port (no alignment rules) and invisible for the whole life of
  the project. `ste/src/game.h` keeps the Card-holding members together at
  the front so they all land on even offsets.
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

- **CBM-510**: unlike every other Commodore port here, `vid510.c`/
  `snd510.c` never poke video/color RAM or SID registers directly. The
  CBM-II's 6509 CPU is bank-switched, and those all live in a separate
  "system bank" plain pointers can't reach -- direct pokes at the
  documented addresses ($F000 screen, $D400 color RAM) never showed up on
  screen, and probing that bank via VICE's monitor gave inconsistent
  results. cc65's `conio.h` (`clrscr()`/`cputcxy()`/`textcolor()`) handles
  the bank-switching correctly internally, so video goes through that
  instead; SID sound uses `pokebsys()`/`peekbsys()` directly (confirmed
  audible), since conio has no sound equivalent. `<cbm510.h>`'s own
  `COLOR_RED`/`COLOR_YELLOW`/etc (the hardware palette) also collide with
  `cards.h`'s suit-color constants of the same name, so `vid510.h`
  hardcodes every color/character value instead of including it.

- **C64 OS**: unlike every other port here, this isn't cc65/C at all -- C64
  OS is a closed-source, commercial GUI operating system for the C64 with
  its own KERNAL, windowing system, and menu bar, and applications are
  written in 6502 assembly against its own TMP-syntax SDK (vendored,
  MIT-licensed, under `inc/os/`, from
  [OpCoders-Inc/c64os-dev](https://github.com/OpCoders-Inc/c64os-dev)).
  The officially-recommended assembler, TurboMacroPro, only runs natively
  *inside* C64 OS itself; cross-assembling from macOS/Linux needs its
  cross-platform sibling, TMPx (`c64os/Makefile` downloads it
  automatically -- there's no native arm64 build, but the Intel macOS one
  runs fine under Rosetta 2). TMPx resolves `.include` paths relative to
  the *current directory*, not its `-I` flag, despite `-I` existing and
  looking like it should work -- `c64os/os` is a symlink to `c64os/inc/os`
  purely so `.include "os/h/whatever.h"` resolves from the project root,
  matching the same workaround in the upstream
  [c64os-example-app](https://github.com/woodrowbarlow/c64os-example-app)
  this port's build pipeline is based on. Testing requires your own
  purchased copy of C64 OS (`c64os/rom/c64os.dhd`) and a CMD HD Boot ROM
  (`c64os/rom/cmd_hd_bootrom.bin`), neither of which is included here --
  see `c64os/README.md`.

- **ZX Spectrum**: another non-6502/non-cc65 port, this time Z80 via
  z88dk. Builds straight to an `.sna` snapshot instead of a `.tap` tape
  image so MAME can load it instantly with `-snapshot` -- typing `LOAD`
  as literal ASCII into a `.tap` autoboot doesn't work the way you'd
  expect, since Sinclair BASIC's keyboard is tokenized (the `J` key alone
  produces the whole word `LOAD` in command mode) and MAME's natural
  keyboard autotype doesn't account for that, so it ends up typing
  `LET OAD` instead of loading anything. `wait_vsync()` originally polled
  the ROM's `FRAMES` system variable the standard way (updated by its
  50Hz interrupt handler), but enabling interrupts crashed within a few
  frames -- the C runtime's stack apparently isn't laid out somewhere
  safe for the ROM's interrupt handler to push onto -- so it paces with
  z88dk's `in_pause()` busy-wait instead, which never touches interrupt
  state. See `zxspectrum/README.md` for the rest (attribute-clash color
  handling, the keys-as-joystick control scheme).
