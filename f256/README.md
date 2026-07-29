# UNO for the Foenix F256K

Written in C against [cc65](https://cc65.github.io/) (65C02), rendering
cards as solid colour tiles in the F256's **Vicky** per-cell-colour text
mode. The F256K is a modern 65C02 machine from Foenix Retro Systems, so
like the Commander X16 and the Atari VBXE ports here, cards are colour
tiles with a contrasting value character rather than color-letter labels.

## Requirements

`cl65`/`cc65` on your `PATH`, and an F256 emulator or real hardware. This
port was developed against a MAME fork with an `f256k` driver; the game is a
**PGZ** loaded off the SD card by FoenixMCP.

## Building and running

```sh
make                 # build/uno.pgz
```

Then put `uno.pgz` on the F256's SD card, boot to SuperBASIC, and launch it
with pexec:

```
/- uno
```

(`/-` hands off to pexec, which loads and runs `uno.pgz`.) `make run` will,
on macOS, copy the PGZ onto a working copy of an SD-card image and launch
MAME for you — point it at your setup:

```sh
make run MAME_DIR=/path/to/mame-foenix256k SDCARD=/path/to/sdcard.img
```

Controls are **keyboard**: `,` and `.` move the selection, **space** (or
return) plays the selected card, **U** draws, and `1`-`9`/`0`/`A`-`J` jump
straight to a hand slot. On the title screen press space to deal; a wild
card's colour picker uses `,` / `.` then space.

## What makes this port different

The **Vicky** video chip has two screen matrices — a character matrix and a
colour matrix — both mapped at `$C000` and selected by paging the 8K I/O
window with `MMU_IO_CTRL` (`$0001`): the char matrix when it's `2`, the
colour matrix when it's `3`, and the Vicky control registers (at `$D000`+)
when it's `0`. A colour-matrix byte is `(foreground << 4) | background`,
each nibble indexing a 16-entry CLUT (`$D800` foreground, `$D840`
background, BGRx). `f256vid.c` programs those CLUTs to the UNO palette and
draws cards as suit-coloured fills with white value characters.

- **I/O-page discipline.** Because the char/colour matrices share the I/O
  window with the Vicky registers, the driver switches `$0001` to 2/3 to
  write cells and back to 0 afterward — and brackets each write batch with
  `sei`/`cli`, because the FoenixMCP kernel's keyboard IRQ expects I/O page
  0 and would read matrix RAM instead of its registers if it fired mid-write.
- **Keyboard via kernel events.** Input polls the kernel's event queue
  (`NextEvent`, non-blocking — carry set means empty), returning
  `event.key.ascii`. `crt0` allocates the event struct and points the kernel
  at it; `f256input.c` just reads it.
- **PGZ packaging.** The linker (with `f256jr_pgz.cfg` + `pgz.s` from the
  FoenixMCP kernel/DOS repo) emits a PGZ directly: code at `$2000`, a `Z`
  header segment, and a zero-size final segment marking the entry point.
- **Sound is a real SID.** The F256 has an onboard left/right SID pair, and
  the left one sits at `$D400`-`$D4FF` — register-for-register identical
  to a 6581/8580, at the same base address the C64 uses. `$D400` is in the
  fixed I/O page 0 alongside the Vicky registers, so unlike the char/colour
  matrix it needs no page-switching to reach. `f256snd.c` is a close port
  of the C64 port's `sid.c` (same voices, waveforms, filter sweeps), with
  the C64's VIC-raster frame pacing swapped for `wait_vsync()`.

The toolchain glue in `toolchain/` (the `f256jr_pgz.cfg` linker config,
`pgz.s`, `kernel.c` POSIX/console layer, `api.h`, and `f256jr.lib`) is
vendored from [ghackwrench/F256_Jr_Kernel_DOS](https://github.com/ghackwrench/F256_Jr_Kernel_DOS).

`make smoke` builds `build/uno-smoke.pgz`, a static render test (title +
a row of colour-tile cards) used to bring the video layer up.

Verified end-to-end in the `f256k` MAME: title, dealt table with
colour-tile cards, hand selection, card play, CPU turns, SID sound, the
wild colour picker, and reverse/skip handling.
