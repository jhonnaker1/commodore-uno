# Commodore UNO

UNO for classic 8-bit Commodore machines: 1 human player vs. 3 CPU
opponents, full rules (Skip, Reverse, Draw Two, Wild, Wild Draw Four with
challenge), written in C against the [cc65](https://cc65.github.io/) 6502
toolchain and tested in [VICE](https://vice-emu.sourceforge.io/).

Each platform is its own self-contained subdirectory sharing the same card
game logic (`cards.c/h`, `game.c/h`, `ai.c/h`) with a platform-specific
video, sound, and input layer underneath, since the hardware capabilities
vary wildly across this lineup.

| Directory | Machine | Status |
|---|---|---|
| [`c64/`](c64) | Commodore 64 | Complete — custom charset, hardware sprites, SID sound (filter/ring-mod), full card animations |
| [`c128/`](c128) | Commodore 128 (40-column) | Complete — reuses the C64's VIC-IIe path; keyboard input is scanned directly off CIA1 rather than via the KERNAL buffer |
| [`plus4/`](plus4) | Commodore Plus/4 | Complete — TED video/sound, stock font |
| [`pet/`](pet) | Commodore PET 4032 | Complete — monochrome text UI, single-voice VIA beeper, keyboard only (no joystick port) |
| [`vic20/`](vic20) | Commodore VIC-20 (+memory expansion) | **Paused** — an unresolved VIC-chip rendering bug causes large parts of a correct, in-memory screen to not actually display; see [`vic20/`](vic20) for details |

## Building

Each platform directory is independent and builds with `make` (requires
`cc65`/`cl65` on your `PATH`):

```sh
cd c64 && make run     # builds build/uno.prg and launches it in x64sc
cd c128 && make run    # build/uno128.prg in x128
cd plus4 && make run   # build/uno4.prg in xplus4
cd pet && make run     # build/uno.prg in xpet
cd vic20 && make run   # build/uno20.prg in xvic -memory all (known broken)
```

`make` alone just builds; `make clean` removes build artifacts.

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
`A`-`J`.

## Known issues

- **VIC-20**: paused. Game logic and screen memory are verified correct
  (via VICE monitor memory dumps) but the VIC chip's actual rendered
  output omits large portions of that same, frozen-instant memory content
  — not a timing/tearing artifact, not a hang, not a logic bug; genuinely
  unexplained rendering behavior that needs more investigation.
