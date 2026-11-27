# UNO for the NeXT: a NeXTSTEP 3.3 application, built ON a NeXT (or in an
# emulator) with NeXTSTEP's own cc from the Developer tools. The top-level
# `make build-all` skips this port for that reason (see README.md).
#
#   make        builds UNO.app (the application wrapper and its program)
#   make clean
CC = cc
CFLAGS = -O -Wall
LIBS = -lNeXT_s -lsys_s

# The application's icon goes into the program itself, in an __ICON
# segment, as NeXTSTEP's own app makefiles put it: a header naming the
# program and its icon, and the icon (a TIFF made by tools/mkicon.py).
ICONFLAGS = -sectcreate __ICON __header UNO.iconheader -segprot __ICON r r \
            -sectcreate __ICON app UNO.tiff

OBJ = obj/main.o obj/ui.o obj/input.o obj/nextgfx.o obj/nextsound.o \
      obj/cards.o obj/game.o obj/ai.o

# Every object depends on every header: there are few enough that
# rebuilding them all is nothing, and a missed dependency is a stale
# object (it once kept the old silent sound stubs compiled into main.o).
HDRS = src/ai.h src/cards.h src/game.h src/input.h src/nextgfx.h src/nextsound.h src/ui.h

all: UNO.app/UNO

# One rule per file: NeXTSTEP's make has no GNU-style % pattern rules.
obj/main.o: src/main.c $(HDRS)
	@-mkdir obj 2>/dev/null
	$(CC) $(CFLAGS) -c src/main.c -o obj/main.o

obj/ui.o: src/ui.c $(HDRS)
	@-mkdir obj 2>/dev/null
	$(CC) $(CFLAGS) -c src/ui.c -o obj/ui.o

obj/input.o: src/input.c $(HDRS)
	@-mkdir obj 2>/dev/null
	$(CC) $(CFLAGS) -c src/input.c -o obj/input.o

obj/cards.o: src/cards.c $(HDRS)
	@-mkdir obj 2>/dev/null
	$(CC) $(CFLAGS) -c src/cards.c -o obj/cards.o

obj/game.o: src/game.c $(HDRS)
	@-mkdir obj 2>/dev/null
	$(CC) $(CFLAGS) -c src/game.c -o obj/game.o

obj/ai.o: src/ai.c $(HDRS)
	@-mkdir obj 2>/dev/null
	$(CC) $(CFLAGS) -c src/ai.c -o obj/ai.o

obj/nextsound.o: src/nextsound.m $(HDRS)
	@-mkdir obj 2>/dev/null
	$(CC) $(CFLAGS) -c src/nextsound.m -o obj/nextsound.o

obj/nextgfx.o: src/nextgfx.m $(HDRS)
	@-mkdir obj 2>/dev/null
	$(CC) $(CFLAGS) -c src/nextgfx.m -o obj/nextgfx.o

UNO.app/UNO: $(OBJ) UNO.iconheader UNO.tiff
	@-mkdir UNO.app 2>/dev/null
	$(CC) $(CFLAGS) $(ICONFLAGS) -o UNO.app/UNO $(OBJ) $(LIBS)

clean:
	rm -rf obj UNO.app

.PHONY: all clean
