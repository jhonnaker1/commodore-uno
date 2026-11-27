# UNO for the NeXT

A NeXTSTEP 3.3 application: a window with the game in it, its own icon, a
menu (Info, New Game, Hide, Quit) with an Info panel, sound effects, and
the mouse alongside the usual keys. It's written in
C, with one Objective-C file for the AppKit side, and drawn with Display
PostScript.

It shares `cards.c`/`game.c`/`ai.c` with every other port, and the standard
`cards.h`: unlike the Amiga, the Atari ST, the Mac and the Lisa (all
68000s), the NeXT's 68030 and 68040 read a word at an odd address without
trapping, so it needs no padding.

**Status: playable** with the mouse or the keys, in colour, in the
[BMNeXT](https://github.com/jhonnaker1/bmnext) host simulator (Previous inside)
as a NeXTstation Turbo Color on NeXTSTEP 3.3, sound included, and in the
releases from v1.0.13. Not yet tried: a greyscale NeXTstation, a 68030 NeXT,
OPENSTEP 4.2, and the menu's keyboard shortcuts.

## Building

It is built on a NeXT, or in an emulator, with NeXTSTEP's own `cc` from
the Developer tools (cc-437.2.6, which is gcc 2.5.8):

```sh
make        # UNO.app, the application wrapper and its program
```

The release's `uno-next.tar` is UNO.app ready-built: unpack it on the
NeXT with `tar xf uno-next.tar` and double-click UNO.app. It is packed by
`tools/mktar.py` with fixed dates and owners, and NeXTSTEP's cc builds the
same program byte for byte every time, so the release reproduces.

To build it yourself, copy the `next` folder onto the NeXT first. In Previous or BMNeXT, the
host's shared folder appears at `/Net/nfs`, so a copy there can be built
in place. Start it with `UNO.app/UNO &` in a Terminal, or by
double-clicking `UNO.app` in the Workspace.

The top-level `make build-all` skips this port, because no compiler on
the host can build it yet: modern gcc and clang don't produce 68k Mach-O
for NeXTSTEP. The makefile has a rule per file because NeXTSTEP's `make`
has no `%` pattern rules, and every object depends on every header.

The icon, `UNO.tiff`, is made by `tools/mkicon.py` (run on the host:
`python3 tools/mkicon.py UNO.tiff`). The NeXT has no Python, so the TIFF
is kept in the repository beside the script.

## Playing

**Click** a card to play it, or the deck to draw; in the colour picker
click a colour, and at a challenge click Yes or No. The arrow keys (or
`,` and `.`) move along the hand, Return or space plays, and up or `U`
draws; `1`-`9`, `0` and `A`-`J` play the card in that slot. New Game
(Command-n) deals again at any point, and Quit (Command-q) or closing the
window ends it.

## How it works

- **The Mac port, translated.** The layout, the words and the game loop
  are the classic Mac port's (`mac/`): `main.c` and `input.c` are nearly
  unchanged, and `ui.c` swaps QuickDraw for a handful of drawing calls in
  `nextgfx.m`. The view draws the Mac's 496x296 layout half as big again
  by scaling (`setDrawSize::` on a flipped view), so the coordinates and
  the hit-testing carry over as they are.
- **The game runs inside the application.** `main()` sets up the
  application and its menu and calls `[NXApp run]`; the delegate's
  `appDidInit:` then calls the game, which never returns. The game's
  loop calls `wait_vsync()` once a frame, and that is where the AppKit's
  event loop lives: it flushes the window, then takes events until the
  next 1/60 s, with `getNextEvent:waitFor:threshold:` and `sendEvent:`.
- **Drawing.** Rounded cards are PostScript arcs (`PSarct`), a wild
  card's four quarters are clipped to its rounded outline, and the text is
  Helvetica. The window is buffered: drawing goes to its off-screen copy
  and reaches the screen when `wait_vsync()` flushes it, so a card is
  never seen half drawn, and the Window Server repaints the window itself
  when something that covered it moves away.
- **The icon** is linked into the program, in an `__ICON` segment, the
  way NeXTSTEP's own app makefiles do it: a header naming the program and
  its icon (`UNO.iconheader`), and a TIFF holding two images of three
  fanned UNO cards, 4-bit colour and 2-bit grey, both with transparency,
  laid out as NeXT's own icons are. The running app's tile and the Info
  panel show it.
- **Sound** is real 16-bit sampled sound. At start-up each effect (the
  Mac port's tunes) is written out as square-wave notes with soft edges,
  in the sound hardware's own 44100 Hz stereo, and played with the C sound
  library's `SNDStartPlaying()`, which returns at once and stops whatever
  is still playing. 22050 Hz mono, which needs the DSP to convert it,
  never finished playing in the emulator.
- **Colour or grey.** On a colour NeXT the suits are the real UNO
  colours. On a greyscale NeXTstation (two bits a pixel) they are four
  greys, as the black-and-white Mac's four patterns, with the suit's
  letter in a corner tab either way.
