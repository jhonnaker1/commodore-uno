# UNO for the classic Macintosh

A real Macintosh application: a window, a menu bar, the mouse, and the
Toolbox event loop, running from a **Mac Plus on System 6** up through the
68k Macs. It shares `cards.c`/`game.c`/`ai.c` with every other port, and
takes the Amiga's padded `cards.h` unchanged (the 68000 again, and gcc
again, so the ST's `game.h` reordering for vbcc isn't needed).

**Status: playable,** with the usual Mac furniture: an About box, a Game
menu (New Game, ⌘N), an Edit menu for desk accessories, and its own
Finder icon, colour on Macs that can show it, and Sound Manager sound
effects. Not yet in a release.

## Building

Needs [Retro68](https://github.com/autc04/Retro68), the gcc cross-compiler
for classic Macs. Build its 68k toolchain once:

```sh
git clone --recursive https://github.com/autc04/Retro68.git
mkdir Retro68-build && cd Retro68-build
../Retro68/build-toolchain.bash --no-ppc --multiversal
```

It builds natively on Apple Silicon (GCC 16 knows how to run on
aarch64-darwin), using Homebrew's boost, cmake, bison, flex, gmp, mpfr,
libmpc, texinfo and hfsutils. Then point `RETRO68` at its `toolchain`
directory:

```sh
make RETRO68=/path/to/Retro68-build/toolchain
```

That produces:

- `build/UNO.dsk` — an 800K floppy image with nothing on it but UNO. This
  is the one to use: put it in drive 2 with any System 6 or 7 startup disk
  in drive 1, and double-click UNO.
- `build/UNO.bin` — the same application as MacBinary, for copying onto a
  real Mac's own disk.

`make run` boots MAME's `macplus` with a startup floppy (`SYSDISK=`) in
drive 1 and UNO in drive 2. Both are copied into `build/` first, since the
Finder writes to every disk it mounts. `MACHINE=macii` runs it on a Mac II.

## Playing

Click a card to play it, or click the deck to draw. Wild cards ask for a
colour, and a Wild Draw Four played on you asks whether to challenge:
click the answer. The keyboard does everything too — the arrow keys (or
`,` and `.`) move, Return plays, `U` draws, and `1`–`9`, `0`, `A`–`J` play
the card in that slot. **⌘N** (Game ▸ New Game) deals a fresh game at any
point, and **⌘Q** quits.

## How it works

- **One binary from a Plus up.** It sticks to the original Toolbox:
  `GetNextEvent` + `SystemTask` rather than `WaitNextEvent`, and a plain
  1-bit `BitMap` for drawing offscreen rather than a GWorld, which needs
  32-bit QuickDraw.
- **The event loop hides in `wait_vsync()`.** Every port's game loop is a
  busy loop calling `wait_vsync()` once a frame. Here that function runs the
  Toolbox event loop until the 1/60 s tick changes — keys, clicks, menus,
  window drags, desk accessories, update events — so `main.c` is the Amiga
  bitmap build's game loop almost unchanged and never knows it's running
  under a cooperative GUI (`src/macgfx.c`).
- **Drawing is offscreen.** `ui.c` draws into a 496×296 1-bit bitmap and
  marks the rows it changed; the next `wait_vsync()` copies just those rows
  to the window. An update event (a desk accessory moved off the window,
  say) is answered by copying the bitmap again, not by redrawing the game.
- **Suits are patterns, or colours when the Mac has them.** On a 1-bit
  screen suits are told apart the way a black-and-white Mac would: red is
  solid black, blue dark grey, green mid grey, yellow light grey. Colour is
  taken only when it is all there — Color QuickDraw, 32-bit QuickDraw (for
  `NewGWorld`: the 6.0.x extension, or System 7) and a main screen set to
  16 colours or more — and then the window is a colour one, the offscreen
  buffer a GWorld at the screen's depth, and the suits the real UNO
  colours, with the card back's red oval and yellow lettering. A Mac II
  with its monitor set to black-and-white gets the patterns. Either way the
  suit's letter sits in a white tab in the corner, wild cards are quartered
  into all four suits, and the value sits in the white oval of a real UNO
  card.
- **Sound is the Sound Manager's square-wave synth** (`noteSynth`), on
  every Mac from System 6.0. One channel is opened at start-up and each
  effect queues a few notes on it and returns, so a tune plays while the
  game carries on; a new effect cuts off the last. Notes are MIDI numbers,
  shaped after the Amiga's effects: blips for plays and draws, rising runs
  for UNO and a win, a falling pair for a failed challenge.
- **Two rows of ten.** At 512×342 twenty whole cards fit, so unlike the
  Amiga and ST fans nothing overlaps. Past twenty the rows scroll with the
  cursor, with `<` and `>` at the sides.
- **New Game is a `longjmp`.** The menu command arrives inside the event
  loop, which may be deep inside a CPU turn or a prompt. Rather than thread
  an "abandon" flag through the shared game loop, `main.c` jumps straight
  back to the top of it; nothing needs unwinding, since `game_new()`
  rebuilds all of `GameState` and the UI redraws from scratch.
- **Resources are its own.** `src/uno.r` replaces Retro68's default with a
  `SIZE` (a 384K partition rather than Retro68's default megabyte), a
  `vers`, and a Finder bundle with the icon `tools/mkicon.py` draws: two
  fanned cards with "UNO" in the oval. Rez writes Finder flags of zero, and
  the Finder ignores a bundle without the hasBundle flag, so
  `tools/setbundle.py` sets it in the MacBinary header (and fixes its CRC)
  before the floppy is built from it with hfsutils.
- **The build is reproducible.** Rez and hfsutils both stamp the build
  time into what they write; `setbundle.py` and `tools/fixdates.py` pin
  every such date (the app's, the volume's and its root folder's) to one
  fixed date, so `UNO.bin` and `UNO.dsk` come out byte-identical every
  time, like the other ports' release assets.
- **The About box is drawn in code** in a plain modal window, so there are
  no dialog resources, and its own small event loop keeps answering the
  game window's update events while it is up.
- **The mouse goes through `ui_hit()`.** The UI knows what it last drew, so
  it answers "what did this click hit?" for `input.c`: a hand card, the
  deck, a colour, Yes or No, or "anywhere" on the title and game-over
  screens. Input queued while a CPU player is thinking is dropped, so a
  stray click can't answer the next question.
