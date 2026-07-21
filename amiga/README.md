# UNO for the Commodore Amiga

Two builds of the same game, sharing `cards.c`/`game.c`/`ai.c`:

- **`make`** → `build/uno` — the **text** build. Draws colored characters
  through console.device on a custom 8-colour Intuition screen (see the
  top-level README for the story behind owning a screen just to get a real
  palette, and the 816×240 window-size archaeology).
- **`make bmp`** → `build/unobmp` — the **bitmap-graphics** build. Renders
  the whole game as pixel-art cards in a custom 320×256, 16-colour lores
  screen, matching the look of the Atari ST and X16/VBXE bitmap builds.

Both need [bebbo's `m68k-amigaos-gcc`](https://github.com/AmigaPorts/m68k-amigaos-gcc)
on your `PATH`. To run either, mount a host folder containing the binary as
a hard drive in [FS-UAE](https://fs-uae.net/) (or WinUAE) with a Kickstart
2.0+ ROM and launch it from the AmigaDOS shell — no disk image needed.

## The bitmap build

Where the text build draws characters, this owns the screen's bitmap and
draws into it with graphics.library (`src/gfx.c`):

- **Card art is fill-based**, and the fills are `RectFill()` — which the
  Amiga runs on the **blitter**, so the card bodies, borders, and centre
  badges are hardware-accelerated rectangle fills rather than CPU loops.
  Unlike the Atari ST port (which hand-masks four bitplanes itself), the
  planar framebuffer is entirely the OS's problem here.
- **Text is the topaz ROM font** via `Text()` in `JAM1` (transparent) mode,
  positioned by baseline (+6 from the glyph top). No embedded font needed —
  it's in Kickstart.
- **The custom screen** is 320×256 lores, depth 4 (16 colours), its palette
  set with `SetRGB4`. `ShowTitle(scr, FALSE)` hides the screen's drag bar so
  the full height is the playfield.
- **Cards** are a white body with a suit-coloured border (a suit fill with a
  3px-inset white fill over it), corner values, and a centre suit badge; the
  hand is a fanned overlap with the selected card lifted and yellow-framed —
  the same design as the ST/X16/VBXE bitmap builds, so `ui_bmp.c` is close
  to a direct sibling of the ST's `ui_ste.c`.

`src/main_bmp.c` is the game loop (identical logic to the text `main.c`),
and `src/input.c`/`amigasound.c` are shared unchanged — input is still
IDCMP `IDCMP_VANILLAKEY`, so movement stays on comma/period and `U` (the
cursor keys were unreliable through the console path; see the top README).

Verified in FS-UAE (A1200, Kickstart 3.1): title screen, dealt table +
fanned hand, card play, multiple CPU turns, UNO/thinking messages.
