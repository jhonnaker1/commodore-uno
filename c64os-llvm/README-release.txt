UNO for C64 OS
==============

The card game, as a C64 OS application: C64 OS's menu bar and status line
stay on screen, and Game > New Game and Game > Quit are in the menu.

This is the C build. The same game rules run on the twenty-odd other
machines in the commodore-uno project, compiled from one source.

    https://github.com/jhonnaker1/commodore-uno


WHAT YOU NEED

C64 OS, on a Commodore 64 or 64C. No RAM expansion is needed.


INSTALLING IT

This zip holds unoc.car, a C64 OS archive, and this file.

Copy unoc.car onto your C64 OS volume, into //os/applications/, and
double-click it there: C64 OS unpacks it and a unoc folder appears. Or
unpack it anywhere and move the unoc folder into //os/applications/.

Then open unoc from Applications, as any other C64 OS app.


PLAYING

You against three CPU players. Cursor left and right pick a card, SPACE or
RETURN plays it, and cursor up draws one. Or play a card directly with the
key shown on it: 1-9, 0, then A-J.

A wild card asks for a colour and a Wild Draw Four played on you asks
whether to challenge it: cursor left and right choose, SPACE or RETURN
confirms.

Each CPU move is shown as it happens, with SID sound effects. Keys pressed
while the CPUs are playing are ignored rather than saved up.


WHAT IS IN THE FOLDER

    main.o        the game
    menu.m        the menu C64 OS shows for the app
    about.t       what C64 OS says about it


ABOUT THIS BUILD

The project has two C64 OS builds. This one is compiled from C by llvm-mos,
from the same game-rule source every other port uses; the other
(uno-c64os.zip) is written in 6502 assembly. They install side by side, as
unoc and uno.

TESTED IN VICE, with C64 OS on a CMD HD image, JiffyDOS and a 16MB REU, and
played through several games. Not yet on real hardware.
