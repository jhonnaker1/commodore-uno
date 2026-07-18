# UNO for the Atari 800XL

Written in C against [cc65](https://cc65.github.io/) (the `atarixl` target).

## Requirements

`cl65` on your `PATH` (from cc65). Running the standard build also needs
`atari800` and an Atari XL/XE OS ROM image (copyrighted Atari firmware, not
included here).

## Building and running the standard build

```sh
make                              # build/uno.xex
make run XLXE_ROM=/path/to/atarixl.rom
```

Standard Atari text mode (ANTIC mode 2 / GRAPHICS 0), 40x24, one shared
foreground/background color for the whole screen -- cards are shown as
bracketed `[label:COLORLETTER+VALUE]`, same trick as the VIC-20/PET/ZX
Spectrum ports.


## VBXE build (WORKING)

`make all-vbxe` builds `build/unovbxe.xex`, a second variant targeting an
Atari 800XL fitted with a [VBXE](https://vbxe.atari.org/) (Video Board XE)
card -- a real add-on FPGA video board with its own 512KB VRAM and a proper
char+attribute text mode, unlike stock Atari text mode's single shared
fg/bg. See `src/vbxevid.c`/`.h` for the driver and `src/ui_vbxe.c` for the
UI built on top of it (80-column NORMAL-width text mode).

**Status: fully working, at C64-port feature parity.** Cards render as
solid 3x3 colored tiles (red/yellow/green/blue, wild in dark gray) with
the value centered in a contrasting color, not a bracketed color-letter --
VBXE's per-cell attribute allows a real colored card face, using dedicated
`TILE_*` palette indices (see vbxevid.h/.c) since the attribute format
ties every foreground color to its own fixed background. On top of that,
matching the C64/C128 ports' feature set:

- **Toss animation**: playing a card animates a small colored block from
  the hand slot to the discard pile, redrawn cell-by-cell each frame
  (`animate_toss_to()` in `main_vbxe.c`) -- VBXE has no hardware sprites,
  so this is the redraw-based equivalent of the C64 port's sprite motion.
- **Deal-in animation**: at the start of each hand, cards animate one at a
  time from the draw pile into the hand grid (`animate_deal()`).
- **Legal-move highlighting**: on the human's turn, cards that can't be
  played on the current top card are grayed out (a dedicated `TILE_DIM`
  palette entry), so the playable cards stand out at a glance -- only the
  VBXE's per-cell color makes this possible, and it heads off the "why
  won't it let me play that card?" confusion. Full color returns during
  CPU turns, when playability isn't a live question.
- **Selection highlight**: the selected card is flanked by solid
  full-height colored bars (`draw_sel_bars()`) that pulse between bright
  and dim rather than blinking fully on/off, so the current selection is
  always unmistakable -- a much stronger cue than the earlier single
  bracket characters.
- **Win-screen flourish**: a rainbow band of `*` characters cycles above
  and below the win message (`ui_win_flourish_step()`).

Verified end-to-end in AltirraSDL (native macOS Altirra) driven through
AltirraBridge: title screen with tile legend, dealt hand in real suit
tiles, toss animation on play, CPU turns, and the win flourish (confirmed
both visually and via a direct VRAM read-back of the "YOU WIN!" text and
flourish colors, to rule out a stale-screenshot artifact).

### Running it

The verified recipe uses the AltirraBridge headless server (a scriptable
native build of Altirra) -- no Wine involved:

```sh
AltirraBridgeServer --bridge=tcp:127.0.0.1:0   # prints a token-file path
```

then from Python (the `altirra_bridge` SDK that ships with the bridge):

```python
from altirra_bridge import AltirraBridge
a = AltirraBridge.from_token_file("<token-file printed by the server>")
a.config("memory", "64K")                    # NOT 320K -- see notes below
a.config("kernel", "/path/to/atarixl.rom")   # real XL OS ROM
a.device_set("vbxe", True, version=126, base="d600")
a.boot("build/unovbxe.xex")
a.frame(300)
a.screenshot(path="/tmp/uno.png")            # use path mode, not inline
```

Two configuration requirements (both discovered the hard way):

- **64K memory, not a RAMBO-style expansion.** cc65's `atarixl` runtime
  aborts with *"This program needs an XL machine"* under 320K RAMBO
  configs -- the expansion's PORTB bank bits break its XL detection.
- **A real Atari XL OS ROM** rather than the AltirraOS substitute.

`make run-vbxe` still contains the older Altirra-via-Wine launch (needs a
GUI-configured `XL_VBXE` profile and the `/gdi` software-renderer flag,
since Wine/MoltenVK renders Altirra's accelerated output as a solid
color). It works, but the bridge recipe above is far more reliable.

### Bugs found on the way (a VBXE-bringup field guide)

Everything below was a real bug, found and fixed in this order. The
recurring theme: **the widely-circulated 2009 VBXE v1.0-beta manual
describes registers that do not exist on the FX core** everyone (and
Altirra) actually implements. When in doubt, trust working source code
(Cactus browser, ansivbxe, Fusik's st2vbxe, Mad Pascal's vbxe unit) or
Altirra's own `vbxe.cpp` -- not the old manual.

1. **Program/VRAM window collision**: cc65's default load address ($2400)
   sits inside the MEMAC CPU window. Linked with `--start-addr 0x4000`.
2. **ATASCII vs screen codes**: the ROM font is indexed by Atari internal
   screen-code order; writing ATASCII bytes directly into VBXE screen
   memory indexes the wrong glyphs. `to_screen_code()` translates.
3. **Palette registers**: the FX core has direct `CSEL/PSEL/CR/CG/CB` at
   `$D644-48`; the old manual's `MSEL/MB0-3` protocol on the same
   addresses scrambles every write. (Confirmed in Altirra's vbxe.cpp.)
4. **OVATT palette select**: byte 1 bits 4-5 pick the Overlay palette.
   Programming palette 1 but leaving OVATT at palette 0 renders from an
   unprogrammed palette. Use `$11` (palette 1 + NORMAL width).
5. **Per-byte long math**: computing MEMAC bank/offset with unsigned-long
   divide/modulo per byte made a screen clear take ~600 frames (looked
   like a hang). Fixed with a one-time bank select + direct window
   pointer (`SCREEN_WIN`).
6. **The big one -- MEMAC**: the old manual's `MA_CPU` register at `$D64C`
   **does not exist** on the FX core. Altirra's register-write switch has
   no case for `$4C`; writes are silently dropped, the CPU window never
   opens, and every "VRAM write" lands in plain Atari RAM instead (which
   also makes read-backs through the same window look correct!). The FX
   protocol is `MEMAC_CONTROL` (`$D65E`: high nibble = window base page,
   bit3 = CPU enable, bits0-1 = window size 4K<<n) plus `MEMAC_BANK_SEL`
   (`$D65F`: bit7 = enable, bits0-6 = VRAM bank in 4KB units). This
   driver uses an 8K window at $2000 (`MEMAC_CONTROL=$29`).

Non-bugs that burned debugging time: Altirra's text-mode XDL handling is
correct (a single `TMON` entry with `RPTL = rows*8-1` steps OVADR every
8th scanline exactly per spec -- verified in `vbxe.cpp`), and the
"nondeterministic" Wine renders were the cc65 XL check failing under
320K memory, not display bugs.

### VRAM layout

All in MEMAC bank 0 (VRAM `$0000-$1FFF`): XDL at `$0000`, font at `$0800`
(CHBASE=1), screen at `$1000` (80x24 char+attr pairs). The XDL is two
entries: an 8-scanline `OVOFF` border entry that programs
OVADR/CHBASE/OVATT, then a single `TMON` entry with `RPTL=191` covering
all 24 text rows, then `END`.

### Possible polish

- The font copy in `vbxe_init` still uses the slow per-byte path (~1s);
  switch it to the direct-window fast path like the screen functions.
- Opponent hands are still shown as plain "CPUn: NN" text rather than a
  tile/pile graphic -- consistent with how every other port here shows
  opponents, so likely not worth changing, but an option if desired.

`src/test_vbxe.c` is a minimal standalone display test (not part of the
game):

```sh
cl65 -t atarixl -O --start-addr 0x4000 -o build/test_vbxe.xex src/test_vbxe.c src/vbxevid.c
```

## VBXE bitmap-graphics build (`make all-vbxe-bmp`)

A separate build of the **full playable game** that renders on VBXE in a
**real 320x192, 8bpp chunky bitmap** (`build/unovbxebmp.xex`) instead of the
text overlay above. The text VBXE build (`unovbxe.xex`) is left untouched.
Where the text overlay draws cards as attribute-colored *cells*, this drives
a linear framebuffer with a 256-entry, 24-bit palette, so cards are
full-color **pixel art** -- white body, drop shadow, a suit-colored border,
corner values, and a center suit badge -- and the hand is a **fanned
overlap** (each card shows its corner strip; the selected card lifts and
gets a yellow highlight; spacing auto-tightens so a full 20-card hand
fits). All text is the Atari ROM charset blitted glyph-by-glyph into the
framebuffer. Same rules, joystick/keyboard input, and POKEY sound as the
text build -- only the render layer differs (no hardware-sprite toss or
blink cursor; the graphics UI redraws on change).

```sh
make all-vbxe-bmp        # build/unovbxebmp.xex (the full bitmap game)
make run-vbxe-bmp        # ... and launch it in Altirra (Wine)
make all-vbxe-bmp-demo   # build/unovbxebmpdemo.xex (static card-art demo)
```

The layers: `src/vbxebmp.c`/`.h` is the driver + card art, `src/ui_bmp.c`
the UI (implements the same `ui.h` the text `ui_vbxe.c` does), and
`src/main_game_vbxebmp.c` the game loop (the same turn logic as
`main_vbxe.c`). `src/vbxebmp_smoke.c` is the original static render
prototype the driver was grown from (the `-demo` target).

Verified end-to-end in AltirraBridge (real `atarixl.rom`, VBXE device):
title screen, dealt table + fanned hand, cursor movement, wild-card color
picker, and full turns against the three CPUs.

Notes from bringing up the bitmap mode:

- **Selecting SR (bitmap) overlay mode.** The VBXE overlay's mode is picked
  by the XDL, per Altirra's `vbxe.cpp` `kOvModeTable`: the graphics entry
  needs the **GMON** control bit (`xdl1 & 3 == 2`) rather than text's TMON,
  with `xdl1` bit2 clear (SR, not HR) and END/mode-column 0. `OVSTEP` is the
  per-scanline byte stride (320); OVADR auto-steps by it each scanline in
  bitmap mode, so the framebuffer is a plain linear 320x192 buffer in VRAM.
- **CPU reach into VRAM.** Same MEMAC 8K window at `$2000` as the text
  driver, banked. The 320x192 framebuffer is 60KB but it sits at VRAM
  `$1000..$FFFF` -- entirely within the low 64K -- so pixel addressing uses
  fast **16-bit** `unsigned int` math and only the bank register selects
  which 8K slice the window shows. `fb_fill16()` does a bank-crossing
  `memset` for horizontal spans.
- **Keeping the draw fast enough to be interactive.** Two things matter for
  a game that redraws on every input: (1) the hot paths avoid 32-bit `long`
  math (the framebuffer fitting under `$10000` is what makes that possible),
  and (2) **card borders are fill-based, not per-pixel outlines** -- a card
  is a suit-colored fill, a white body fill inset by 3px, a fill for the
  centre badge, plus three small ROM-font glyphs, so almost all of the work
  is fast `memset` runs and only the glyphs plot pixel-by-pixel. (The first
  prototype outlined borders with `vbmp_pixel` and a full screen took
  seconds; the fill-based version redraws the table + hand fast enough to
  play.)
