# UNO for the classic 68k Macintosh, built with Retro68
# (https://github.com/autc04/Retro68) -- gcc targeting m68k-apple-macos.
#
# RETRO68 is the toolchain prefix that Retro68's build-toolchain.bash
# installs (the `toolchain/` directory inside its build directory).
RETRO68 ?= /Volumes/Retro68/Retro68-build/toolchain
CC = $(RETRO68)/bin/m68k-apple-macos-gcc
REZ = $(RETRO68)/bin/Rez
RINCLUDES = $(RETRO68)/m68k-apple-macos/RIncludes

CFLAGS = -O2 -Wall -ffunction-sections
LDFLAGS = -Wl,-gc-sections -Wl,--mac-strip-macsbug

SRC = src/main.c src/macgfx.c src/ui.c src/input.c src/macsound.c \
      src/cards.c src/game.c src/ai.c
HDR = $(wildcard src/*.h)

# The 800K floppy holding only the app (what ships), and the same app as
# MacBinary for transferring to a real Mac.
OUT_DSK = build/UNO.dsk
OUT_BIN = build/UNO.bin

# `make run` boots a Mac Plus in MAME from a System 6/7 startup floppy in
# drive 1 with UNO's floppy in drive 2; double-click UNO in the Finder.
# Both are copied into build/ first, because the Finder writes to every
# disk it mounts and the originals should stay untouched.
MAME ?= $(HOME)/mame0281-arm64/mame
MAMEDIR ?= $(HOME)/mame0281-arm64
SYSDISK ?= /Volumes/Retro68/mactest/sys608.img
MACHINE ?= macplus

.PHONY: all run clean

all: $(OUT_DSK)

build/UNO.code.bin: $(SRC) $(HDR)
	@mkdir -p build
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(SRC)

# Resources (src/uno.r: SIZE, version, Finder icon and bundle) + the code,
# as MacBinary. Rez writes Finder flags of zero, so setbundle.py then sets
# hasBundle -- without it the Finder ignores the icon.
$(OUT_BIN): build/UNO.code.bin src/uno.r tools/setbundle.py
	$(REZ) src/uno.r -I$(RINCLUDES) \
	  --copy build/UNO.code.bin -o $@ -t APPL -c UNOg
	python3 tools/setbundle.py $@

# The floppy is built from the MacBinary with hfsutils (Retro68 ships a
# copy) so it carries those Finder flags. hmount keeps its state in
# $$HOME/.hcwd, so HOME points into build/ rather than at yours. hfsutils
# stamps the volume with the build time; fixdates.py pins those dates (as
# setbundle.py does the app's) so the image reproduces byte for byte.
$(OUT_DSK): $(OUT_BIN) tools/fixdates.py
	rm -f $@
	dd if=/dev/zero of=$@ bs=1024 count=800 2>/dev/null
	HOME=$(CURDIR)/build $(RETRO68)/bin/hformat -l UNO $@ >/dev/null
	HOME=$(CURDIR)/build $(RETRO68)/bin/hmount $@ >/dev/null
	HOME=$(CURDIR)/build $(RETRO68)/bin/hcopy -m $(OUT_BIN) :UNO
	HOME=$(CURDIR)/build $(RETRO68)/bin/humount
	python3 tools/fixdates.py $@

run: $(OUT_DSK)
	cp "$(SYSDISK)" build/run-system.img
	cp $(OUT_DSK) build/run-uno.dsk
	cd $(MAMEDIR) && $(MAME) $(MACHINE) \
	  -flop1 $(CURDIR)/build/run-system.img -flop2 $(CURDIR)/build/run-uno.dsk \
	  -skip_gameinfo -window

clean:
	rm -f build/UNO.* build/run-* build/.hcwd
