# UNO for MS-DOS

1 human player vs. 3 CPU opponents, full rules (Skip, Reverse, Draw Two,
Wild, Wild Draw Four with challenge) — the same game logic as every other
port in this repo (`cards.c`, `game.c`, `ai.c` are shared verbatim), with a
CGA/PC-speaker/BIOS-keyboard layer underneath.

Built for the **8088 in real mode**, so the binaries run on a stock 1981 IBM
PC or PC/XT — and on every PC since, plus DOSBox and FreeDOS. Nothing here
needs a 286, not even the graphics builds: a video card is not a CPU.

Three builds, one codebase, picked by what card you have:

| Build | Video | Screen |
|---|---|---|
| `UNO.EXE` | CGA 80x25 colour text | colour card tiles, per-cell foreground and background |
| `UNOEGA.EXE` | EGA mode 10h | 640x350, 16 colours — pixel-art cards, the highest resolution in this repo |
| `UNOVGA.EXE` | VGA mode 13h | 320x200, 256 colours — pixel art again, with the suits programmed to the real UNO colours |

## Controls

| Key | Action |
|---|---|
| ← → | Move the hand cursor |
| ↑ ↓ | Move between the two rows of ten |
| `SPACE` / `ENTER` | Play the selected card, or confirm |
| `U` | Draw a card |
| `1`-`9`, `0`, `A`-`J` | Jump straight to that hand slot |
| `ESC` | Quit to DOS |

Real arrow keys, which most machines in this repo cannot manage — the
Amiga, F256K and TI-99/4A ports all fall back to comma/period because their
arrows either don't reach the program or arrive as control codes.

## Why CGA text, and the one interesting thing about it

CGA gives per-cell foreground *and* background colour, the same model the
X16, VBXE, F256 and MEGA65 tile ports use, so cards are drawn the same way:
solid blocks of the suit colour with the value knocked out. What CGA adds
over those machines is width — 80 columns is twice the C64's 40 and two and
a half times the MSX2's 32, so the whole hand fits as two straight rows of
ten with no overlapping fan.

The catch is in the attribute byte. It has only three bits for background,
so backgrounds are normally limited to the eight dark colours, and yellow
would come out brown. Bit 7 is the blink flag — and turning blinking off
re-purposes it as the background intensity bit, making all sixteen colours
available as backgrounds. That is what lets the four suits be the real UNO
red/yellow/green/blue.

CGA and VGA want that done differently, so `cga_init()` detects which it is
on (INT 10h AH=12h/BL=10h leaves BL alone on anything that is not EGA or
better) and then either calls the BIOS (`INT 10h AX=1003h`, EGA/VGA) or
writes CGA's mode register at `0x3D8` directly. Poking `0x3D8` on a VGA is
not reliable — that register is not part of the VGA spec.

## CGA snow

The first working build had coloured speckles flickering across the screen
during CPU turns. That is not a bug in the code or the emulator — it is a
genuine defect in the IBM CGA card, and DOSBox-X reproduces it faithfully.

The 6845 CRTC and the CPU share the regen buffer at `B800:0000` with no
arbitration between them. When the CPU writes while the card is actively
fetching display data, the CRTC loses the cycle it needed and puts garbage
on screen for it. It afflicts the 80-column text modes on genuine IBM CGA;
clones fixed it, and EGA/VGA never had it.

It showed up on CPU turns because that is the largest burst of writes — the
opponents row, the table, the whole hand and the message line, back to back.

Bit 0 of the status port at `0x3DA` is 1 exactly when a regen-buffer access
can be made without disturbing the display, so `poke_cell()` waits for that
before every write. It waits out any window already in progress before
catching the start of the next one, so the write lands with a whole interval
ahead of it rather than at the tail of one.

That costs a poll loop per cell, which is why it is done only where it is
needed: `cga_init()` already works out whether the machine is EGA or better
(for the blink setting), and anything later than CGA writes at full speed.

This is worth knowing about mostly because it is invisible on modern
hardware. Developing against VGA — or against a less accurate emulator —
would never have surfaced it, and the port would have snowed on exactly the
machine it is named for.

## EGA and VGA: two backends, one UI

The graphics builds share `ui_bmp.c` and differ only in which backend
implements `gfx.h`. They are about as far apart as PC graphics hardware
gets, and comparing them is most of the interest:

**EGA mode 10h is planar, and not in the way the Amiga and Atari ST ports
are.** Those two are planar as well, but there the planes are ordinary CPU
memory the program addresses itself. EGA's four planes all sit behind
`A000:0000` at once, and which of them a write reaches — and what value it
writes — is decided by registers in the Graphics Controller, not by the
address. A CPU write does not so much carry data to the card as trigger the
card's own ALU.

`egavid.c` drives it with Set/Reset: the Set/Reset register holds a colour,
Enable Set/Reset is on for all four planes, and the byte the CPU writes is
then ignored as data — each plane takes the corresponding bit of the colour
instead. The Bit Mask register picks which of the eight pixels in the byte
change; the rest keep what the latches hold, which is why a byte must be
*read* first (a read loads all four latches). One byte-write therefore
paints up to eight pixels a solid colour across all four planes at once,
which suits card tiles exactly.

**VGA mode 13h is the opposite trade, and is startlingly simple**: a pixel
is a byte, `screen[y*320+x] = colour`. No planes, no latches, no
registers, no masks. It is the easiest video hardware in this entire repo —
easier than the C64's — and it arrived last.

So EGA gets nearly four times the pixels, and VGA gets a real palette: 256
entries chosen from 262,144, so `vgavid.c` programs the four suits to the
actual UNO colours rather than picking the nearest of a fixed sixteen. The
EGA build reprograms exactly one entry, the felt, from the default green
(0,170,0) to a secondary-intensity green (0,85,0) — a dark table colour
plain CGA cannot make at all.

One thing did *not* scale between them, and it is worth recording because
the layout was written to derive everything from `GFX_W`/`GFX_H`: the ROM
font is 8 pixels wide in both modes. EGA is therefore 80 characters across
and VGA only 40. Vertical positions derive from the screen height happily,
but text does not — three opponent labels reading `CPU1 CARDS: 7` need 39
of VGA's 40 columns, and the help line needs more than it has. `ui_bmp.c`
picks shorter wording below 60 columns, and sizes the table's columns by
the wider of the card and its label, since at 640 the card is wider and at
320 the label is.

## Sound

The PC speaker: one square-wave voice gated by timer channel 2 of the 8253.
Its input clock is 1.193182MHz, which is 14.31818MHz divided by 12 — and
14.31818 is four times the 3.579545MHz NTSC colourburst. That makes this the
third machine in the repo whose tone constant comes off that same crystal,
after the TI-99/4A's SN76489 and the MSX2's AY-3-8910.

## Building

```sh
make          # build/UNO.EXE     (CGA text)
make ega      # build/UNOEGA.EXE  (640x350x16)
make vga      # build/UNOVGA.EXE  (320x200x256)
make img      # build/uno.img, a 1.44M FAT12 floppy with all three
make run      # runs the CGA build in DOSBox-X
make run-ega  # ... the EGA build
make run-vga  # ... the VGA build
```

`WATCOM` points at the toolchain (default `~/dos-toolchain`), `DOSBOX` at
the emulator.

### Building the compiler

Open Watcom is the only actively-maintained compiler that still targets
16-bit real-mode DOS, and it isn't packaged for macOS — but unlike the
TMS9900 and vbcc toolchains, building it on Apple Silicon is a supported
configuration with no patches needed. The host binaries come out native
arm64; no Rosetta.

```sh
git clone https://github.com/open-watcom/open-watcom-v2.git
cd open-watcom-v2
cp setvars.sh setvars-mac.sh
```

Edit `setvars-mac.sh` and change two lines — the defaults are for Linux:

- `export OWTOOLS=CLANG` (from `GCC`), to build with the host's clang.
- `export OWNOWGML=1` (uncomment it), which suppresses the documentation
  build. That step wants a DOSBox to run the old WGML typesetter under,
  which is a bootstrapping problem you don't need for a compiler.

Then:

```sh
. ./setvars-mac.sh
./build.sh          # compiles everything (~30 min on an M3)
./build.sh rel      # collects the scattered output into rel/
mv rel ~/dos-toolchain
```

The second step is easy to miss and looks like a failure if you do: plain
`./build.sh` exits 0 having built the compilers into *per-component*
`binbuild/` subdirectories (`bld/cc/i86/osxa64/binbuild/wcc.exe` and so on),
with no `rel/` tree anywhere. `./build.sh rel` is what gathers them,
together with the headers and the DOS libraries, into one prefix. Once
that's moved out, the 2.4GB checkout can be deleted.

Sanity check, which is worth doing before deleting anything:

```sh
export WATCOM=$HOME/dos-toolchain
export INCLUDE=$WATCOM/h
export PATH=$WATCOM/armo64:$PATH
wcl -0 -ms -bcl=dos -fe=HELLO.EXE hello.c
```

## Running

`make run` uses DOSBox-X with `-machine cga`. It is not cycle-accurate for
an XT, but it is scriptable — `-silent` plus `-c "PROG > OUT.TXT"` against a
mounted host directory gives real captured program output, which is a much
better debugging channel than the screenshot-only loop the other ports get.

For genuine 4.77MHz 8088 timing, load `build/uno.img` as floppy A: in
[86Box](https://86box.net/) with machine = IBM XT and video = CGA. `make
run-xt` prints the settings.
