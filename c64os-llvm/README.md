# UNO for C64 OS, in C

A real windowed [C64 OS](https://c64os.com/) application, like the one in
[`../c64os`](../c64os), with one difference that matters to this repo: the
game rules are the shared `cards.c`/`game.c`/`ai.c` that every other C port
compiles, **byte-identical** and enforced by `make check` at the repo root,
rather than a hand port to assembly. A rules fix lands here the same day it
lands everywhere else.

It is built by [llvm-mos](https://llvm-mos.org/). C64 OS has no C toolchain
of its own, but an application is only a contract -- a `$0900` PRG, five
vectors and a KERNAL link table -- and llvm-mos can produce that. The
[ega trek](https://github.com/jhonnaker1/8x8-Trek) project proved the toolchain with a
339-byte experiment; this is the first complete game built that way.

| | [`../c64os`](../c64os) | `c64os-llvm` |
|---|---|---|
| Language | 6502 assembly (TMPx) | C (llvm-mos) + a small assembly shim |
| Game rules | `engine.s`, hand-ported | the shared `cards.c`/`game.c`/`ai.c` |
| Sound | none | SID effects, from the bare C64 port |
| `main.o` | 7,036 bytes | 13,091 bytes |
| Installs as | `uno` | `unoc` -- both can be installed at once |

## Requirements

* The [llvm-mos SDK](https://github.com/llvm-mos/llvm-mos-sdk/releases),
  prebuilt for macOS, Linux and Windows. `LLVM_MOS` defaults to
  `$HOME/llvm-mos`.
* `python3` (the `menu.m`/`about.t` generator; no packages needed).

To install and play in VICE: `x64sc`, `petcat` and `c1541` (all part of
VICE), plus three things you have to own, exactly as for `../c64os`: a
purchased C64 OS hard disk image, a CMD HD boot ROM and a JiffyDOS KERNAL.
They default to `../c64os/rom/` (`c64os.dhd`, `cmd_hd_bootrom.bin`,
`JiffyDOS_C64.bin`); point elsewhere with `make run C64OS_ROMS=/path/to/dir`.

## Building and running

```
make           # build/uno/ -- the bundle: main.o, menu.m, about.t
make d64       # build/uno-c64os-llvm.d64 -- the bundle on a disk image
make zip       # build/uno-c64os-llvm.zip -- the release asset, a unoc/ folder
make install   # copy it onto C64 OS's disk, then verify it byte by byte
make run       # boot C64 OS; double-click "unoc" in //os/applications/
```

`make install` works on `build/c64os.dhd`, a copy of your disk, never the
original; delete the copy to start over from a clean C64 OS. It runs a BASIC
installer in warp mode that copies the three files from VICE's host
directory (device 8) onto the CMD HD (device 10), then reads each one back
and compares it with the source, printing `main.o ok 11223` -- or the offset
of the first difference. Close VICE when it has printed all three, then
`make run`.

**Why the installer checks itself.** An install interrupted part-way leaves
a zero-block, never-closed `main.o` on the disk, and C64 OS's response to
that is silent: the File Manager unloads, nothing loads, and the File
Manager comes back. It looks exactly like an app that crashed on launch.
That cost three launches before a directory listing showed `0 "main.o" *prg`.

On a real C64 with C64 OS: copy the three files from `build/uno/` (or the
`.d64`, or the `unoc/` folder in the release zip, whose timestamps are fixed
so it rebuilds byte for byte) into a new folder under `//os/applications/`. `main.o` is a PRG;
`menu.m` and `about.t` must be SEQ, like every app C64 OS ships.

## Controls

The same as the assembly port: **cursor left/right** to pick a card,
**space** or **return** to play it, **cursor up** to draw, or play directly
by the key on the card -- `1`-`9`, `0`, then `A`-`J`. The colour picker and
the Wild Draw Four challenge are cursor left/right and space/return. **Game
> New Game** in the menu bar starts a new game; **Game > Quit** leaves.

Keys pressed while the CPUs are playing are dropped rather than queued: they
were pressed against a board the player had not seen yet.

## How it works

`src/app.s` is the C64 OS side: the five vectors at `$0900`, the layer
(draw and keyboard callbacks), the KERNAL link table C64 OS rewrites into
`JMP`s, and the trampolines between the OS and C. `src/main.c` is the game:
a small state machine driven by keyboard events, drawing straight into C64
OS's screen and colour buffers (`$0400`/`$D800`) -- the same buffers the
assembly port blits its layer into -- so there is no syscall per character.
`src/sid.c` is the bare C64 port's sound effects. `c64os.ld` places
everything.

## What C64 OS asks of compiled C

Each of these was a failure first, and each was measured before it was
fixed.

* **Zero page.** llvm-mos keeps 32 imaginary registers in zero page. C64
  OS's own memory map -- `//os/docs/memory.t`, on the C64 OS disk -- shows
  it uses most of zero page itself, and the first choice, `$02`-`$21`, is
  its timers, exception handler and screen-layer pointers. Swapping those
  bytes at every OS/C crossing is not enough: **C64 OS's interrupt handler
  decrements `$02` and `$09` while C is running**, which moved C's soft
  stack pointer under it and scattered stray bytes through the program. The
  registers now sit at **`$4E`-`$6D`** -- BASIC's floating-point
  accumulators, used by C64 OS only inside its float library -- and a crawl
  of every instruction reachable from C64 OS's IRQ and NMI vectors, in a
  RAM snapshot of the running system, confirms none of it touches that
  range. The swap stays, for the OS calls.
* **Everything inside the loaded file.** C64 OS allocates the pages an
  app's file covers, and no more, so `.bss` and the soft stack are part of
  the file rather than `NOLOAD` sections after it. Loading them as zeros
  also stands in for crt0's zero-bss, which `-nostartfiles` drops.
* **The soft stack is 512 bytes and uses 34**, read out of memory after
  several rounds of play (`app.s` fills it with `$A5` at startup).
* **The draw callback runs with I/O banked out** (`$01` = `$34`), so `$D800`
  there is C64 OS's colour *buffer* in RAM, not colour RAM. The key and menu
  handlers run with it in (`$36`), so drawing and sound each set `$01` for
  what they touch and put it back.
* **The buffers are not the screen.** The VIC shows the RAM under I/O at
  `$DC00` (`$DD00` = `$C4`, `$D018` = `$75`), and C64 OS copies its buffers
  there at the end of each event-loop pass. A chain of CPU turns runs
  inside one key event, so drawing each move into the buffers showed
  nothing until the chain ended; `os_present` in `app.s` does the copy
  itself. (C64 OS's own `redraw_` would do it by calling back into C from
  inside C, which llvm-mos's static stack frames do not allow for.)
* **`volatile` is not optional.** `src/sid.c` is the bare C64 port's, which
  cc65 builds without `volatile` on its registers. Compiled unmodified by
  llvm-mos, `sfx_draw` became `jmp .` -- the raster wait folded into an
  infinite loop -- so the first card drawn would have hung the machine.
* **A false lead worth recording:** a CPU-history trace taken after the
  zero-page corruption had already started suggested C64 OS calls apps with
  the hardware stack nearly full. A snapshot of a clean entry showed SP =
  `$FB` -- nearly empty. A trace of a corrupted run is not evidence about a
  clean one.

## Tested

In VICE 3.x (`x64sc`, CMD HD, JiffyDOS, 16MB REU): launched from the File
Manager, several complete rounds played, sound heard, CPU turns seen one at
a time, and **Game > New Game** and **Game > Quit** both used. The built
`main.o` is reproducible byte for byte and identical to the binary that was
played. Not individually exercised yet: a hand beyond 20 cards (slots past
`J` have no quick key and are reached with the cursor), and the
empty-draw-pile path.

## Differences from the assembly port

* **The Wild Draw Four challenge prompt.** Here, choosing `[yes]` challenges.
  In `../c64os`, `challenge_sel` (0 for `[yes]`) is passed straight to
  `resolve_wd4`, where 0 means *no challenge* -- so `[yes]` declines there.
* Sound, and each CPU move shown as it is made. The assembly port calls its
  `drawmain` directly between CPU turns, which fills C64 OS's buffers but
  (per the note above) does not reach the screen until the turns are over.
* A line saying what the last player did, and what it caused (skip,
  reverse, draw, UNO).
* Hands of up to 40 cards are drawn in full, 6 to a row.
