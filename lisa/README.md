# UNO for the Apple Lisa

A bare-metal boot floppy for the Lisa 2. UNO takes over the machine the
way the 8-bit ports do: no Lisa Office System, no MacWorks. It's written in C
and compiled with [Retro68](https://github.com/autc04/Retro68)'s m68k gcc,
using its plain ELF linker rather than the Mac application one.

It shares `cards.c`/`game.c`/`ai.c` with every other port and takes the
Amiga's padded `cards.h` (the 68000 again), like the Mac and the ST.

**Status: playable with the mouse or the keyboard** in the IDLE emulator
(inside the [BMLisa](https://github.com/jhonnaker1/bmlisa) host simulator), with sound effects on the Lisa's
speaker. The look is the black-and-white Mac port's: patterned suits with
a letter tab, quartered wild cards, two rows of ten in the hand. It has
only been run in emulation, never on a real Lisa, and IDLE emulates no
sound, so the effects are untested. Not in a release yet.

## Building

```sh
make RETRO68=/path/to/Retro68-build/toolchain
```

`build/UNO.dc42` is a 400K Disk Copy 4.2 floppy image, the format
LisaEm and IDLE read. On a Lisa set to start from its hard disk, press
**Apple-3** (Startup From…) and then **Apple-2** (the floppy). With no
hard disk the ROM stops at error 85; the same keys work there.

## Running it

```sh
make run
```

builds BMLisa's host simulator if needed and runs UNO in it in real
time, in a web page that opens at `http://127.0.0.1:8064/` (click the
page to give it the mouse; Ctrl+C in the terminal stops it). It needs a
BMLisa checkout (`BMLISA`, default `~/claude-code/BMLisa`) and MAME's
`lisa2.zip` (`LISAROMS`, default `~/mame0281-arm64/roms/lisa2.zip`), and
takes `PORT` too. The simulator's SD card is `build/run`: the three Lisa
2 ROMs copied out of the zip, a copy of the floppy, and a parameter
memory set to start from the floppy (`tools/mkpram.py`), so UNO comes up
by itself in about 50 seconds.

## Playing

**Click** a card to play it, or the deck to draw; in the colour picker
click a colour, and at a challenge click Yes or No. Any click starts a
game or the next one.

The Lisa keyboard has no arrow keys. **,** and **.** (or the keypad's
**4** and **6**) move along your hand, and **Return**, **Enter** or the
space bar plays the card. **U** (or keypad **8**) draws. **1**–**9**,
**0** and **A**–**J** play the card in that slot directly. In the colour
picker and at a challenge, the same keys move between choices and Return
picks one.

## How it boots

Everything here was read from the boot ROM (341-0175-H / 341-0176-H,
disassembled) and IDLE's source, not guessed:

- **The boot ROM** reads floppy block 0 into `$20000`, with its 12 tag
  bytes just below it at `$1FFF4`. It starts the block only if the tag's
  file ID (bytes 4–5) is `$AAAA`, then jumps to `$20000` in supervisor mode.
- **The rest of the program** follows in blocks 1, 2, … and loads
  straight on from `$20200`. The boot block (`src/boot.S`) reads them
  through the floppy controller's shared memory at `$FCC001`, using the
  ROM's own sequence. The command, drive, side, sector and track go into
  the shared memory, and `$81` in the go byte runs the command. When the
  read is done, FDIR (VIA1 port B bit 4) goes up. Then `$85` with `$CC`
  clears it, and the 512 data bytes sit in the odd bytes from `$FCC401`.
  The linker works out how many blocks there are (`__load_blocks`).
- **The ROM leaves** the screen's logical address at `$110` (the top 32K
  of RAM, 720×364 at 90 bytes a row) and the end of RAM at `$2A8`. The
  stack goes just under the screen. Interrupts stay off and everything is
  polled.
- **Keyboard and mouse** bytes come from the COPS through VIA1 port A
  (`$FCDD83`), with CA1 in the flag register (`$FCDD9B`, bit 1) saying a
  byte is waiting. A key is `drrrnnnn` (bit 7 down, then the key number):
  `b` sends `$EE` down and `$6E` up. The mouse button is key `$06`, so it
  sends `$86` when pressed. Mouse movement comes as `$00 dx dy`, and
  `$80` starts a reset or clock report (`$80 $2F` at power-on).
- **Starting from the floppy.** The startup device is the high nibble of
  parameter memory byte 4 (`$FCC189`; the parameter memory is the odd
  bytes of `$FCC181`–`$FCC1FF`), and 1 is the floppy. The ROM trusts it
  only if the 32 words pass its checksum: add each word to a total and
  rotate the total left a bit, and it must come out 0.
- **Floppy geometry:** 400K Lisa/Sony disks have 12 sectors a track on
  tracks 0–15, 11 on 16–31, down to 8 on 64–79, one side.

## How it draws

- **No OS, no interrupts.** The program is polled from start to finish.
  `wait_vsync()` waits for the vertical retrace bit in the MMU status
  register (`$FCF801` bit 2), which the video interrupt enable at
  `$FCE01A` arms, and `$FCE018` clears. It reads the keyboard while it
  waits (`src/lisagfx.c`).
- **Tall pixels.** A Lisa pixel is about 1.5 times as tall as it is wide.
  So cards are 64×66 pixels, about the shape of a real card on the
  screen, with corners 9 across and 6 down. Most text is drawn at double
  width.
- **Its own shapes and font.** There's no QuickDraw, so `lisagfx.c` fills
  rectangles, rounded rectangles and ovals row by row in the Mac's
  patterns. Text uses a 5×9 font, upper and lower case, generated from
  ASCII art by `tools/mkfont.py`.
- **Only what changed is redrawn.** A card takes about a tenth of a
  second to draw at 5 MHz, and the shared game loop asks for the whole
  table, the opponents and the hand again on every turn. So `ui.c`
  remembers what each part shows (every hand slot, the discard, the deck
  count, the colour, the play order, each opponent's box) and redraws
  only the parts that differ. `span()`, which every shape is drawn with,
  finds rows from a table rather than by multiplying, and fills four
  bytes at a time.
- **The mouse pointer** is drawn straight onto the screen, never into
  the drawing buffer. Each frame it comes off (the bytes under it are put
  back from the buffer), the changed rows are copied, and it goes back on.
  The COPS reports movement as `$00 dx dy`; the boot ROM has already
  turned the mouse on.
- **A tiny C library** (`src/libc.c`): the start-up that clears BSS, the
  string and memory routines, and `rand()`.

## How it sounds

The speaker hangs off the keyboard VIA (`$FCDD81`), and `src/lisasound.c`
drives it the way the boot ROM's own beep does (`$FE0B06`):

- Port B bits 1–3 set the volume (the ROM uses 4 of 7).
- Bits 2–4 of the auxiliary control register at 100 run the shift
  register free, sending its byte to the speaker over and over at the rate
  timer 2's low latch sets. `$0F` makes one square wave a byte. Clearing
  the bits stops it.
- The VIA runs at 500 kHz, so a note is 500000 / (16 × (N + 2)) Hz.
  Higher notes use `$33` (two cycles a byte) to tune more finely.

An effect queues its notes and returns. `wait_vsync()` moves to the next
note each frame, so the game never waits for a sound. The tunes are the
Mac port's, an octave up, because the lowest note this way is about
122 Hz.
