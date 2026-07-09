# UNO for the ZX Spectrum

Written in C against [z88dk](https://github.com/z88dk/z88dk) (the `zx`
target, zsdcc compiler, `sdcc_iy` C library) -- Z80, not 6502, so this is
a genuinely different toolchain from every other 8-bit port in this repo.

## Requirements

Download a z88dk release from the
[releases page](https://github.com/z88dk/z88dk/releases) (the macOS build
works fine on Apple Silicon under Rosetta 2 if needed, but is also
available as a native arm64 binary) and extract it somewhere, e.g. `~/z88dk`.

## Building

```sh
make Z88DK_DIR=/path/to/z88dk   # build/uno.sna
make Z88DK_DIR=/path/to/z88dk run   # boots it in MAME's spectrum driver
```

`Z88DK_DIR` defaults to `~/z88dk` if not given. `make run` also needs
`MAME` on your `PATH` and a ZX Spectrum 48K ROM at
`~/mame0281-arm64/roms/spectrum/spectrum.rom` (or override `MAME`/
`SPECTRUM_ROM`) -- the ROM is copyrighted and not included here.

## Notes

- Builds straight to an `.sna` snapshot rather than a `.tap` tape image --
  MAME loads it instantly with `-snapshot`, no `LOAD ""` needed. Typing
  `LOAD` as literal ASCII into a `.tap` autoboot doesn't work the way
  you'd expect: Sinclair BASIC's keyboard is tokenized (the `J` key alone
  produces the whole word `LOAD` in command mode), so MAME's natural
  keyboard autotype ends up typing `LET OAD` instead of loading anything.
  Snapshots sidestep the whole problem.
- The screen is 32x24 text over a bitmap, with colour applied per 8x8
  attribute cell (one ink + one paper colour per cell -- the classic
  Spectrum "attribute clash"). There's no per-cell colour independent of
  the glyph and no `cputcxy()`-style direct API; z88dk drives the screen
  through `stdio` with control codes embedded in the string
  (`\x10`=INK, `\x11`=PAPER, `\x14`=INVERSE, `\x16`=AT for position).
  Cards are shown as bracketed `[label:COLORLETTER+VALUE]` (same trick as
  the VIC-20/PET/Atari ports) rather than true per-cell colour, and
  selection uses INVERSE instead of a colour change.
- No dedicated cursor keys exist on a real Spectrum keyboard, and 1-9/0
  are already the quick-play keys (same convention as every other port
  here), so movement uses the classic `O`/`P`/`Q` "keys as joystick"
  scheme a lot of commercial Spectrum games used, with SPACE or ENTER to
  play/confirm.
- `wait_vsync()` originally polled the ROM's `FRAMES` system variable
  (updated by its 50Hz interrupt handler) -- the standard Spectrum way to
  pace a loop. Enabling interrupts crashed within a few frames, most
  likely the C runtime's stack isn't laid out somewhere safe for the
  ROM's own interrupt handler to push return addresses onto. Paces with
  z88dk's `in_pause(20)` busy-wait instead, which never touches interrupt
  state at all.
