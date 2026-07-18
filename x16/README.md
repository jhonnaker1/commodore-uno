# UNO for the Commander X16

Written in C against [cc65](https://cc65.github.io/) (the `cx16` target),
with one small assembly file for frame timing.

## Requirements

`cl65` on your `PATH` (from cc65) and the [x16emu
emulator](https://github.com/X16Community/x16-emulator) (with its `rom.bin`)
to run it. Real hardware works too.

## Building and running

```sh
make                                   # build/uno.prg
make run X16EMU=/path/to/x16emu_dir    # launches build/uno.prg in x16emu
```

`make run` defaults `X16EMU` to `/Users/jhonnaker/x16emu_macos_m1-r48`;
point it at wherever your emulator (and its `rom.bin`) live. To run by hand:

```sh
cd /path/to/x16emu_dir
./x16emu -prg /path/to/uno/x16/build/uno.prg -run
```

## What makes this port different

The X16's video chip, **VERA**, has a real char + per-cell color text mode:
each screen cell carries its own foreground *and* background palette index
(4 bits each, from a reprogrammable 256-entry, 12-bit palette). So, like the
Atari VBXE port and unlike the color-letter ports (VIC-20/PET/Atari/Spectrum),
cards render as solid colored **tiles** with a contrasting value character.

This driver talks to VERA directly (through its auto-incrementing data port)
rather than going through cc65's `conio`, which buys full per-cell background
control. On top of the tiles it matches the C64/VBXE feature set:

- **Colored card tiles** — solid suit-colored faces (`x16vera.c`'s
  `cell_color[]` table maps each logical color to a VERA `bg<<4|fg` byte).
- **Legal-move highlighting** — on the current top card, cards you can't
  play are drawn in a *darkened* version of their suit color (a handful of
  palette slots are reprogrammed to dark-suit shades in `vera_init()`), so
  the playable cards stand out; the whole hand keeps its colors even when
  nothing is playable, and the prompt switches to "PRESS UP TO DRAW".
- **Selection highlight** — the selected card is flanked by solid
  full-height bars that pulse bright/dim.
- **Hardware-sprite card toss** — the X16 has 128 hardware sprites, so
  playing (and dealing) glides a *real* VERA sprite (a suit-colored card)
  from pile to hand, not a redraw-based block (`spr_show`/`spr_move`/
  `spr_hide` in `x16vera.c`, driven by `animate_toss_to()` in `main.c`).
- **VERA PSG sound** — the full set of effects on PSG voice 0
  (`x16snd.c`).

Input is joystick (cc65's `cx16` joystick driver, SNES pad) or keyboard
(cursor keys + space/return, or `1`-`9`/`0`/`A`-`J` to jump to a card).

## Notes from bringing it up

- **Direct VERA, not conio.** cc65's `conio` renders text on the X16, but
  its per-cell color model is thinner than writing VERA yourself; direct
  writes to the text map at VRAM `$1B000` (128-cell stride, `char,color`
  pairs) give independent per-cell foreground and background, which is what
  the solid tiles and the dark-dim highlighting need.
- **Screen codes.** The X16 boots in the PETSCII upper/lowercase font;
  `vera_init()` switches to upper/graphics (`CHR$(142)`) so screen codes
  1-26 render as uppercase A-Z, matching `to_screen_code()`.
- **Frame timing.** cc65's `cx16` target has no `waitvsync()`, so
  `vsync.s` polls VERA's VSYNC interrupt-status flag with interrupts
  masked (SEI), so the KERNAL's own vsync IRQ can't clear the flag out from
  under the poll.
- **Palette for dimming.** VERA text cells can only reference palette
  indices 0-15 for background, so the dark-suit "dimmed" colors are made by
  reprogramming five otherwise-unused default palette slots (3/4/9/10/13)
  to dark red/yellow/green/blue/gray.

Verified end-to-end in x16emu (title screen, dealt hand in real color
tiles with legal-move dimming, the sprite toss on deal/play, CPU turns).

## Bitmap-graphics build (`make bmp`)

A second, separate build of the same game that renders in a **320x240,
256-colour bitmap framebuffer** instead of text/tiles -- cards are drawn as
pixel art (white body, drop shadow, suit border, corner values, a centre
suit pip), the hand is a **fanned overlap** (each card shows its corner
strip; the selected card lifts and highlights; spacing auto-tightens so a
full 20-card hand fits), and the table/messages are drawn with the KERNAL
font. Same rules, input and PSG sound as the text build -- only the render
layer differs.

```sh
make bmp       # build/unobmp.prg (the full bitmap game)
make run-bmp   # ... and launch it
make bmpdemo   # build/unobmpdemo.prg (a static card-art render demo)
```

Built entirely on the X16 KERNAL's **GRAPH_* framebuffer API** (thin cc65
thunks in `src/graph.s`; `src/vbmp.c` is the drawing + card-art layer,
`src/ui_bmp.c` the UI, `src/main_game_bmp.c` the loop):

- **Entering graphics mode.** `GRAPH_init` alone does *not* switch the video
  mode here -- you must call `screen_mode($80)` (320x240x256) first, then
  `GRAPH_init`.
- **KERNAL 16-bit ABI.** GRAPH takes arguments in the virtual registers
  r0-r4 at zero page `$02-$0A`; cc65's `cx16` ZP starts at `$22`, so C sets
  those directly and the asm thunk just marshals A/X/carry and JMPs the
  `$FFxx` vector.
- **Baseline text.** `GRAPH_put_char` positions a glyph by its *baseline*
  (glyph pixels sit ABOVE the given y), so every erase rectangle that
  precedes changing text (messages, counts) extends ~8px above the baseline
  -- otherwise old glyph tops linger and overlap the new text.

Verified end-to-end in x16emu: title, dealt table + fanned hand, full turns
against the CPUs.
