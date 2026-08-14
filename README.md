# UNO for Vintage Computers

UNO across a lineup of vintage machines spanning six CPU families — 6502,
Z80, 6809, 68000, TMS9900 and x86: 1 human player vs. 3 CPU opponents, full rules
(Skip, Reverse, Draw Two, Wild, Wild Draw Four
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
driver. The Tandy CoCo 3 is a Motorola 6809, built with
[CMOC](https://github.com/stahta01/cmoc) and tested in
[XRoar](https://www.6809.org.uk/xroar/). The TI-99/4A port is a fifth CPU
family — TI's own 16-bit TMS9900 —
built with gcc 4.4 carrying
[mburkley's TMS9900 patches](https://github.com/mburkley/tms9900-gcc) plus
[Tursi's libti99](https://github.com/tursilion/libti99), and tested in MAME's
`ti99_4a` driver. The MSX2 port is Z80 again but a different toolchain and a
much more capable video chip, built with [SDCC](https://sdcc.sourceforge.net/)
and tested in [openMSX](https://openmsx.org/). The MS-DOS port is a sixth
CPU family — Intel x86, the 8088 in real mode — built with
[Open Watcom](https://open-watcom.github.io/) and tested in
[DOSBox-X](https://dosbox-x.com/) and [86Box](https://86box.net/).

Each platform is its own self-contained subdirectory sharing the same card
game logic — `cards.c`, `game.c` and `ai.c` are byte-identical across all
of them — with a platform-specific video, sound, and input layer underneath,
since the hardware capabilities vary wildly across this lineup. Their
headers are shared too, with one deliberate exception: the two 68000 ports
pad and reorder the `Card` struct to satisfy the 68000's alignment rules,
which the byte-addressable CPUs neither need nor can afford (the padding
would double `GameState`). See the Atari ST/STE entry under Notes.

**Prebuilt binaries** for every platform are attached to the
[latest release (v1.0.5)](https://github.com/jhonnaker1/commodore-uno/releases/tag/v1.0.5)
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
| [`coco/`](coco) | Tandy Color Computer 3 | Complete — a fourth CPU family (Motorola 6809), built with [CMOC](https://github.com/stahta01/cmoc) and tested in [XRoar](https://www.6809.org.uk/xroar/); ships as a Disk Basic machine-language file that `LOADM`/`EXEC`s, so no disk image is needed. Keyboard-only, scanned straight off the keyboard matrix. The CoCo 3's GIME actually *does* have true per-character colour via Super Extended Color Basic's `attr()`, unlike the PET/VIC-20/Atari/Spectrum ports — but the cards keep those ports' bracketed `[label:COLORLETTER+VALUE]` convention for consistency, with `attr()` used only for the selection highlight. Its colour arguments are raw GIME text-palette slots rather than RGB, and index 1 turned out to be the same hue as the default background (invisible), so text uses index 4 and the highlight index 7 — see [`coco/`](coco) |
| [`x16/`](x16) | Commander X16 | Complete — the modern 8-bit machine; talks straight to its VERA video chip (per-cell fg+bg color) for solid colored card tiles with legal-move dimming and a pulsing selection highlight, a real **hardware-sprite** card toss (VERA has 128 sprites), and VERA PSG sound — the same feature set as the C64/VBXE ports. Also ships a separate **bitmap-graphics** build (`make bmp`) that renders the whole game in a 320×240 256-color framebuffer via the KERNAL GRAPH API — pixel-drawn card faces and a fanned hand |
| [`ste/`](ste) | Atari ST / STE (68000) | Complete — the only machine here with **no text mode at all**, and its framebuffer is four bitplanes interleaved by word rather than chunky, so the video layer masks whole 16-pixel word groups across all four planes and draws card faces as nested fills to stay at word speed. Full-colour pixel-art cards and a fanned hand in 320×200/16 colours. Writes the palette in **STE 4-bit-per-gun encoding**, whose extra bit the ST simply ignores — 4096 colours on an STE, a graceful fallback to the ST's 512 from one code path — and detects which machine it's on via the `_MCH` cookie. Carries its own 8×8 font (generated by [`tools/genfont.py`](tools/genfont.py)) since the ROM font is only reachable through Line-A/VDI, YM2149 sound via XBIOS `Dosound()`, keyboard input. Built with vbcc, not cc65 — see [`ste/`](ste) |
| [`f256/`](f256) | Foenix F256K | Complete — the first 65C02 Foenix machine here; renders cards as solid colour tiles in the **Vicky** per-cell-colour text mode (like the X16/VBXE tile ports). The Vicky char + colour matrices share the `$C000` I/O window, paged by `MMU_IO_CTRL` (`$0001`); colour bytes are `(fg<<4)\|bg` into 16-entry CLUTs. Built with cc65 as a **PGZ** loaded off the SD card by FoenixMCP (`/- uno` from SuperBASIC), keyboard input via the kernel event queue, real SID sound effects (the F256's left SID sits at `$D400`, register-identical to a 6581, same base address as the C64); developed against a `f256k` MAME driver — see [`f256/`](f256) |
| [`mega65/`](mega65) | MEGA65 | Complete — the modern Commodore-65 recreation; compiles as a `c64`-target program that brings up the VIC-IV video chip via [mega65-libc](https://github.com/mega65/mega65-libc) for a per-cell-color 40-column screen (color-bordered card boxes), a **VIC-II hardware-sprite** card toss on every play, and SID sound directly mapped at `$D400`. (Legal-move dimming and stereo dual-SID are noted follow-ups; 80-column needs native C65 mode — see [`mega65/`](mega65).) |
| [`ti99/`](ti99) | Texas Instruments TI-99/4A | Complete — the only **TMS9900** machine here (16-bit, and a fifth CPU family), built with gcc 4.4 + [mburkley's TMS9900 patches](https://github.com/mburkley/tms9900-gcc) and [libti99](https://github.com/tursilion/libti99). Its TMS9918A has no per-cell colour at all — one 32-byte colour table colours *groups of eight character codes*, so colour belongs to the glyph, not the screen position. The port buys colour back by spending character codes on it: six 24-code ranges (four suits, wild, and a cursor highlight) each hold the same 23-glyph card alphabet in their own colour, giving solid colour card tiles like the X16/VBXE/F256 ports on hardware that nominally can't do them. SN76489 sound (three real voices, so chords rather than beeps). At ~12K it outgrows the 8K cartridge window, so it ships as a **16K two-bank cart whose ROM stub copies the game into the 32K expansion and runs it from RAM** — see [`ti99/`](ti99) |
| [`msx2/`](msx2) | MSX2 (and MSX2+ / turbo R) | Complete — Z80 like the Spectrum port, but via [SDCC](https://sdcc.sourceforge.net/) rather than z88dk and with far better video to aim at. The MSX1's VDP is the same TMS9918A as the TI-99/4A, with the same colour-per-character-code constraint; the MSX2's **V9938** drops it and adds the two things this game wants — a **programmable palette** (SCREEN 5 is 256×212, 16 colours out of 512, so the suits are the real UNO colours rather than the nearest fixed ones) and a **hardware blitter**, which is what makes full pixel-art cards affordable on a 3.58MHz Z80 where the Amiga/ST ports need a 68000 to get the same look. Glyphs come from the machine's own ROM font via the `CGTBL` pointer, so the cartridge carries no font. AY-3-8910 PSG sound — three real voices, so chords; its tone constant `111861/Hz` is identical to the TI-99/4A's SN76489, both being divided down from the same colourburst crystal. Ships as a flat **16K cartridge ROM** (11,296 bytes used, so no bank switching), and runs unchanged on MSX2+ and turbo R — see [`msx2/`](msx2) |
| [`dos/`](dos) | IBM PC/XT and up, MS-DOS | Complete — a sixth CPU family (Intel **x86**), and the widest reach of anything here: built for the **8088 in real mode**, so one binary runs on a 1981 IBM PC, a Pentium, DOSBox and FreeDOS alike. Nothing in it needs a 286. CGA 80x25 colour text gives per-cell foreground *and* background like the X16/VBXE/F256 tile ports, but at twice the C64's width, so the whole hand fits as two straight rows of ten with no overlapping fan. The suits are the real UNO colours only because **blinking is turned off**: CGA's attribute byte has three bits of background, so backgrounds are normally the eight dark colours (yellow comes out brown) — clearing the blink bit re-purposes it as background intensity and unlocks all sixteen. PC-speaker sound (one square-wave voice, so arpeggios not chords), real arrow keys, and ESC quits to the DOS prompt. Ships as a plain `.EXE` and a 1.44M FAT12 floppy image — see [`dos/`](dos) |

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
cd coco && make run CMOC_DIR=/path/to/cmoc  # build/uno.bin in XRoar (needs your own coco3.rom in coco/rom/)
cd x16 && make run X16EMU=/path/to/x16emu_dir  # build/uno.prg in the Commander X16 emulator
cd mega65 && make run XMEGA65=/path/to/xmega65 M65ROM=/path/to/mega65-rom.bin  # build/uno.prg in Xemu's xmega65
cd f256 && make       # build/uno.pgz -- put it on an F256 SD card, see f256/ (make run drives MAME)
cd ste && make run VBCC=/path/to/vbcc TOS_ROM=/path/to/tos.img  # build/UNO.TOS in Hatari as an STE
cd ti99 && make run  # build/uno.rpk in MAME's ti99_4a (needs the tms9900 toolchain, see ti99/)
cd msx2 && make run  # build/uno.rom in openMSX (brew install sdcc; no system ROM of your own needed)
cd dos && make run   # build/UNO.EXE in DOSBox-X emulating CGA (make img for a 1.44M floppy)
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

The Foenix F256K has a similar deployment step for a different reason. It is
cc65 like the Commodore ports, but the F256 has no tape, disk or cartridge
in the Commodore sense — programs are loaded off an SD card by the FoenixMCP
kernel — so `make` produces a **PGZ** rather than a `.prg`:

```sh
cd f256 && make        # build/uno.pgz
```

Copy `uno.pgz` onto the F256's SD card, boot to SuperBASIC, and launch it
with `/- uno` (`/-` hands off to the pexec chainloader, which loads and runs
it). `make run` will, on macOS, copy the PGZ onto a working copy of an
SD-card image and start MAME for you — point it at your own setup:

```sh
cd f256 && make run MAME_DIR=/path/to/mame-foenix256k SDCARD=/path/to/sdcard.img
```

The linker config, PGZ startup and kernel shim under `f256/toolchain/` are
vendored from
[ghackwrench/F256_Jr_Kernel_DOS](https://github.com/ghackwrench/F256_Jr_Kernel_DOS).

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

The TI-99/4A port needs yet another from-source toolchain: gcc 4.4.0 and
binutils 2.19.1 carrying the TMS9900 patches, since nothing packages a
TMS9900 compiler. [`ti99/README.md`](ti99/README.md) has the commands and,
more importantly, the two fixes Apple Silicon needs — the first one is not
optional and not obvious: GCC 4.4 calls every generated RTL pattern function
through a *variadic* pointer, which works only where variadic and fixed
arguments share a calling convention. Apple's arm64 ABI passes variadic
arguments on the stack, so every pattern gets garbage operands and the
compiler segfaults on literally any input, including a one-line `add()`.
One line in `gcc/recog.h` fixes it. Then:

```sh
cd ti99 && make        # build/uno.rpk
```

`make run` launches it in MAME's `ti99_4a` with the 32K expansion attached
(the game runs from that RAM). At the TI title screen press any key, then
`2` to pick UNO.

The MSX2 port is the opposite experience — the least painful toolchain in the
repo, since SDCC is bottled in Homebrew and an MSX cartridge is a plain 16K
binary with a 16-byte header:

```sh
brew install sdcc
cd msx2 && make        # build/uno.rom
```

`make run` boots it in [openMSX](https://openmsx.org/); it needs no system
ROMs of your own, since openMSX ships the free C-BIOS
(`make run MACHINE=C-BIOS_MSX2`). SDCC has no MSX crt0, so
[`msx2/src/crt0.s`](msx2/src/crt0.s) is the entire C runtime. Three SDCC
4.6.0 problems bite this port, all written up in
[`msx2/README.md`](msx2/README.md): an outright internal compiler error, a
silent miscompile that clobbers a live register, and — the interesting one —
`__critical`, whose `ld a,i` interrupt-state save walks straight into a
documented **Z80 erratum**. If an interrupt lands during that one
instruction the saved state reads as "disabled", the matching `ei` is
skipped, and interrupts stay off permanently: the frame counter stops and
the game freezes mid-turn. Plain `di`/`ei` is the fix.

The C64 and C128 versions use a custom character set (real chargen ROM
glyphs for codes 0-127, hand-drawn card-art glyphs above that). The
generated headers are checked in under `src/charset_data.h` /
`src/charset_codes.h`, so a fresh clone builds without needing the ROM.
Regenerating them (`make charset`) requires pointing `CHARGEN_PATH` in
`tools/gen_charset.py` at your own dumped C64 chargen ROM — the ROM itself
isn't included here.


The MS-DOS port needs Open Watcom, the only actively-maintained compiler
still targeting 16-bit real-mode DOS. It is not packaged for macOS, but
unlike the TMS9900 and vbcc toolchains it builds natively on Apple Silicon
with no patches -- the host binaries come out arm64, no Rosetta.
[`dos/README.md`](dos/README.md) has the commands, including the step that
is easy to miss: plain `./build.sh` exits 0 having built the compilers into
per-component `binbuild/` subdirectories with no release tree anywhere, and
`./build.sh rel` is what gathers them into one prefix. Then:

```sh
cd dos && make        # build/UNO.EXE
cd dos && make img    # build/uno.img, a 1.44M FAT12 floppy
```

`make run` boots it in DOSBox-X emulating CGA. Unlike every other port here,
this one can also be checked non-visually: `make run-smoke` renders the
screen, dumps the text buffer and attribute map to a file, and prints it on
the host.

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

One per port, in the same order as the table above — the thing about
each machine that cost the most time to work out.

- **C64**: the reference implementation the other Commodore ports were
  derived from, and the only one with a custom character set.
  `tools/gen_charset.py` builds a 2048-byte chargen in which codes 0-127
  are copied verbatim from a real C64 character ROM, so ordinary PETSCII
  text still renders, and 128-255 are hand-drawn card art -- box corners
  and edges, big digits, action-card icons, a card-back pattern, the
  cursor. That needs a dumped chargen ROM, which isn't redistributable, so
  the generated `charset_data.h`/`charset_codes.h` are checked in and a
  fresh clone builds without one (`make charset` regenerates them if you
  point `CHARGEN_PATH` at your own dump). `vic_init()` then moves the whole
  VIC bank to $8000 through CIA2's port A, putting the screen matrix at
  $8000 and the charset at $8800 -- with the sprite *pointers* at screen
  base + $3F8, tucked into the 24 bytes at the end of the 1K screen block
  that a visible 40x25 matrix leaves unused.

- **C128**: two builds from one codebase. The default reuses the C64's
  VIC-IIe path nearly unchanged (though keyboard input is scanned straight
  off CIA1 rather than through the KERNAL buffer); `make run-vdc` targets
  the 80-column 8563/8568 VDC instead. The VDC bites twice. Its character
  generator wants glyph slots on 16-byte boundaries even though the font is
  only 8 scanlines tall, so glyphs are stored double-spaced; and register
  28's character-base bits get re-asserted by the KERNAL's 80-column cursor
  interrupt, so the driver rewrites them every frame or the charset
  silently reverts. The subtler one was colour: `vdc.h`'s constants had been
  copy-pasted from the VIC-II palette, which the VDC does not share -- it is
  RGBI (bits 3/2/1/0 = red/green/blue/intensity), so `COL_RED` (2) actually
  rendered as dark blue and `COL_YELLOW` (7) as light cyan. That is the
  "only blues and greens, no red or yellow" symptom, and it predated the
  sprite work it first got blamed on.

- **Plus/4**: TED colour bytes are a hue nibble OR'd with a luminance
  nibble rather than a flat palette index, so `ted.h` spells out its own
  `COL_*` values. It can't use cc65's `cbm264.h` macros for them, because
  those collide *by name but not by value* with `cards.h`'s
  `COLOR_RED`/`COLOR_YELLOW`/etc. suit constants -- the same collision the
  CBM-510 port hits with `cbm510.h`, and the same shape of problem as the
  CoCo 3, TI-99/4A and MSX2 ports' `stdlib.h` shims. The shared game logic
  is verbatim across every port, so wherever a platform header disagrees
  with it, the platform side gives way.

- **PET**: the only machine here with no CPU-visible vertical-blank or
  raster signal at all -- there is no VIC-II `$D012` or TED equivalent to
  poll -- so `wait_tick()` paces the game off the KERNAL jiffy clock via
  cc65's `clock()` rather than waiting on the display. It is also the only
  genuinely monochrome target (no colour RAM whatsoever), so cards are
  bracketed colour-*letter* labels; that fallback was later reused by the
  VIC-20, Atari, Spectrum and CoCo ports, which have colour but can't spend
  it per cell cheaply. `make run-8032` builds a separate 80-column
  executable for the PET 8032 -- same hardware family, same screen at
  $8000, just twice the columns -- as a parallel `petvid8032.c`/`ui8032.c`
  pair rather than a runtime switch.

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

- **Atari 800XL**: the stock build is ordinary ANTIC text, but the VBXE
  build produced the most misleading bug in the project. The old VBXE
  manual documents an `MA_CPU` register at `$D64C` for opening the MEMAC
  CPU window onto video RAM. On the shipping FX core that register **does
  not exist**: Altirra's register-write switch has no case for `$4C`, so
  writes are silently dropped, the window never opens, and every "VRAM
  write" lands in plain Atari RAM instead. The trap is that reads back
  through the same window then look *correct*, because they hit that same
  plain RAM -- so the driver appears to work right up until nothing
  appears on screen. The real protocol is `MEMAC_CONTROL` (`$D65E`) plus
  `MEMAC_BANK_SEL` (`$D65F`); this driver runs an 8K window at $2000. It
  was found by reading Altirra's `vbxe.cpp` rather than the documentation,
  which is the lesson: for this hardware the emulator source is the
  specification. Two smaller ones -- the ROM font is indexed in Atari
  internal screen-code order, so writing ATASCII bytes indexes the wrong
  glyphs; and computing MEMAC bank/offset with a `long` divide per byte
  made a screen clear take ~600 frames, which looked exactly like a hang.

- **Apple IIe**: builds with `-D,__EXEHDR__=0` to suppress the
  AppleSingle-style relocation header that cc65's `apple2enh-system.cfg`
  prepends by default. That header exists to be parsed by a smart loader
  (cc65's own `LOADER.SYSTEM`), but a plain `-NAME` ProDOS system launch
  does no parsing at all -- it loads the raw file to $2000 and jumps there,
  which with the header present means jumping straight into header bytes
  instead of code. There is also no `make run` target: cc65 cannot emit a
  bootable ProDOS floppy and no free tool builds one, so
  `apple/tools/make_disk.py` injects the binary into a copy of your own
  ProDOS boot image by overwriting an existing multi-block file's data
  blocks in place.

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

- **CoCo 3**: CMOC does its own preprocessing by shelling out to the host's
  plain `cc -E`, and ships no `stdlib.h` of its own (`rand()`/`srand()` live
  in `cmoc.h`). The shared `cards.c` includes `<stdlib.h>` and is verbatim
  across every port, so on macOS it fell through to the system SDK's header,
  which drags in libc++'s C++ compatibility headers and fails on things
  (`__BYTE_ORDER__`, a bare `__has_builtin`) that have nothing to do with
  this program. `src/compat/stdlib.h` shadows it via an earlier `-I` with
  just `#include <cmoc.h>` -- the same trick the TI-99/4A and MSX2 ports
  later needed. On the video side the GIME really does have true
  per-character colour through `attr()`, unlike the PET/VIC-20/Atari/
  Spectrum ports, but its arguments are raw GIME text-palette *slots*, not
  RGB: checked empirically in XRoar, index 1 on the default background came
  out completely invisible, being the same hue as it, so text uses index 4
  and the highlight index 7.

- **Commander X16**: goes straight at VERA rather than through cc65's
  `conio`, whose per-cell colour model is thinner than the hardware's --
  writing the text map at VRAM `$1B000` directly (128-cell stride,
  `char,colour` pairs) gives independent foreground and background per cell,
  which is what the solid tiles and the dimmed illegal-move cards need. Two
  things needed working around: cc65's `cx16` target has no `waitvsync()`,
  so `vsync.s` polls VERA's own VSYNC interrupt-status flag with interrupts
  masked, or the KERNAL's vsync IRQ clears the flag out from under the poll;
  and a text cell can only name palette indices 0-15 for its background, so
  the dimmed suit colours are made by reprogramming five otherwise-unused
  default palette slots (3/4/9/10/13) rather than by picking darker existing
  ones.

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

- **Foenix F256K**: the Vicky's character matrix and colour matrix are
  *both* mapped at `$C000`, selected by paging the 8K I/O window with
  `MMU_IO_CTRL` (`$0001`) -- 2 for characters, 3 for colour, 0 for the Vicky
  control registers. Every screen write is therefore a page-switch sandwich,
  and each batch is bracketed with `sei`/`cli`: the FoenixMCP kernel's
  keyboard interrupt expects I/O page 0, and firing mid-write would have it
  read matrix RAM where it expected its own registers. Sound, by contrast,
  came nearly free -- the F256's left SID sits at `$D400`, register-for-
  register a 6581 at the same address the C64 uses, and in the fixed I/O
  page 0 where it needs no paging at all, so `f256snd.c` is a close port of
  the C64's `sid.c`.

- **MEGA65**: compiles as a cc65 `c64`-target program that then unlocks the
  VIC-IV through [mega65-libc](https://github.com/mega65/mega65-libc) -- a
  C64 binary that stops being one at `mega65_init()`. The awkwardness is at
  link time: mega65-libc's `random.o` defines its own `rand`/`srand`, which
  collide with cc65's stdlib, and the shared game logic calls `rand()`. The
  Makefile drops `random.o` from the built `libmega65.a` rather than
  patching either side. Text goes through `cputcxy()`, which takes raw
  screen codes, so `mega65vid.c` translates PETSCII on the way out while the
  UI's string literals stay ASCII via `<ascii_charmap.h>`.

- **TI-99/4A**: the only port whose *compiler* had to be debugged before
  the game could be. Nothing packages a TMS9900 compiler, so this is gcc
  4.4.0 with mburkley's patches built from source -- and on Apple Silicon it
  segfaulted on every input, including a one-line `add()`. GCC 4.4 calls
  every generated RTL pattern function through a pointer typed
  `rtx (*)(rtx, ...)`, which only works where variadic and fixed arguments
  share a calling convention. True on x86-64 and on AAPCS64; not true on
  Apple's arm64 ABI, which passes variadic arguments on the stack while the
  callee reads registers -- so every pattern was built from garbage
  operands. One line in `gcc/recog.h` fixes it. The machine then has no
  per-cell colour at all: a single 32-byte table colours *groups of eight
  character codes*, so the port spends character codes to buy colours --
  six 24-code ranges (four suits, wild, and the cursor highlight), budgeted
  across exactly 32 groups. At ~12K it also outgrows the 8K cartridge
  window, so it ships as a two-bank cart whose loader stub -- byte-identical
  in both banks, because switching banks swaps memory out from under the
  program counter -- copies the game into the 32K expansion and runs it
  from there.

- **MSX2**: the same CPU as the Spectrum port and the opposite experience
  of it. Where the Spectrum has to fight attribute clash for colour, the
  V9938 hands over a programmable palette and a blitter, so the cards are
  real pixel art rather than coloured letters. Interrupts are left alone
  and running here -- the BIOS's own handler is what advances the `JIFFY`
  counter `wait_vsync()` waits on, and it is also why R#15 has to be put
  straight back after reading the blitter's busy flag: that handler reads
  status register 0 every frame to acknowledge the VDP interrupt, and
  leaving it pointed elsewhere wedges the machine. See `msx2/README.md`.

- **MS-DOS**: the first working build had coloured speckles flickering across
  the screen during CPU turns. That is not a bug — it is **CGA snow**, a
  genuine defect in the IBM CGA card. The 6845 CRTC and the CPU share the
  regen buffer at `B800:0000` with no arbitration, so a CPU write while the
  card is fetching display data costs the CRTC the cycle it needed and puts
  garbage on screen for it. It afflicts the 80-column text modes on real IBM
  CGA; clones fixed it and EGA/VGA never had it. Bit 0 of the status port at
  `0x3DA` is 1 exactly when an access is safe, so every write waits for that
  first -- but only on CGA, since the poll loop costs a great deal and
  `cga_init()` has already worked out whether the machine is EGA or better
  for the blink setting. Worth recording because it is invisible on modern
  hardware: developing against VGA, or a less accurate emulator, would have
  shipped a port that snowed on exactly the machine it is named for. This is
  also the one port that can be checked without looking at it -- DOS can
  write a file, so `make run-smoke` dumps the rendered text buffer and its
  attributes for the host to read, and a layout bug that had slot numbers
  landing on the "YOUR HAND" label was caught that way rather than by eye.

## License

MIT — see [LICENSE](LICENSE). Use it, port it, put it on a real machine.

Two directories vendor third-party code that keeps its own licence, and those
files are not covered by the above: `c64os/inc/` (the C64 OS SDK headers from
[OpCoders-Inc/c64os-dev](https://github.com/OpCoders-Inc/c64os-dev), MIT, see
[`c64os/inc/LICENSE`](c64os/inc/LICENSE)) and `c64os/tools/` (the TMPx
cross-assembler, see [`c64os/tools/LICENSE`](c64os/tools/LICENSE)).

No machine ROMs are included anywhere in this repo — the ports that need one
(CoCo 3, Atari XL/XE, Atari ST, Amiga, MEGA65) expect you to supply your own.
The MSX2 port needs none: openMSX ships the free C-BIOS, which runs it.
