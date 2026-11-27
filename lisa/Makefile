# UNO for the Apple Lisa: a bare-metal boot floppy, built with Retro68's
# m68k gcc (its plain ELF linker, not the Mac application one).
RETRO68 ?= /Volumes/Retro68/Retro68-build/toolchain
CROSS   := $(RETRO68)/bin/m68k-apple-macos-
CC      := $(CROSS)gcc
LD      := $(CROSS)ld.real
OBJCOPY := $(CROSS)objcopy
LIBGCC  := $(shell $(CC) -print-libgcc-file-name)

CFLAGS  = -m68000 -O2 -Wall -ffreestanding -fno-builtin -fomit-frame-pointer \
          -ffunction-sections -fdata-sections --param=min-pagesize=0
LDFLAGS = -T lisa.ld --gc-sections -nostdlib --no-warn-rwx-segments

SRC = src/main.c src/ui.c src/input.c src/lisagfx.c src/lisasound.c src/libc.c \
      src/cards.c src/game.c src/ai.c
OBJ = build/boot.o $(SRC:src/%.c=build/%.o)

all: build/UNO.dc42

build/%.o: src/%.c | build
	$(CC) $(CFLAGS) -Ibuild -c $< -o $@

# The font is generated from the ASCII art in tools/mkfont.py.
build/lisafont.h: tools/mkfont.py | build
	python3 tools/mkfont.py $@

build/lisagfx.o: build/lisafont.h

build/boot.o: src/boot.S | build
	$(CC) -m68000 -c $< -o $@

build/UNO.elf: $(OBJ) lisa.ld
	$(LD) $(LDFLAGS) -o $@ $(OBJ) $(LIBGCC)

build/UNO.bin: build/UNO.elf
	$(OBJCOPY) -O binary $< $@

build/UNO.dc42: build/UNO.bin tools/mkdisk.py
	python3 tools/mkdisk.py $< $@

build:
	mkdir -p build

# `make run` plays it in BMLisa's host simulator (IDLE, the emulator BMLisa
# runs), in real time in a web page: http://127.0.0.1:$(PORT)/. Its SD card
# is build/run: the three Lisa 2 ROMs copied out of MAME's lisa2.zip, a copy
# of the floppy, and a parameter memory that starts the Lisa from the
# floppy (tools/mkpram.py), so UNO comes up on its own in about 50 seconds.
# Click the page to give it the mouse; Ctrl+C here stops it.
BMLISA ?= $(HOME)/claude-code/BMLisa
LISAROMS ?= $(HOME)/mame0281-arm64/roms/lisa2.zip
PORT ?= 8064
OPEN ?= open
SIM = $(BMLISA)/tools/hostsim/build/bmlisa_sim

run: build/UNO.dc42 tools/mkpram.py
	$(MAKE) -s -C $(BMLISA)/tools/hostsim
	rm -rf build/run
	mkdir -p build/run/roms build/run/disks
	unzip -q -j -o "$(LISAROMS)" 341-0175-h 341-0176-h vidstate.rom -d build/run/roms
	cp build/UNO.dc42 build/run/disks/UNO.dc42
	python3 tools/mkpram.py build/run/bmlisa.pram
	(sleep 2; $(OPEN) http://127.0.0.1:$(PORT)/) &
	cd build/run && BMLISA_LIVE=$(PORT) \
	  BMLISA_CMDLINE="root=$(CURDIR)/build/run harddisk=none floppy=disks/UNO.dc42" \
	  "$(SIM)" < /dev/null

clean:
	rm -rf build/*.o build/UNO.* build/lisafont.h build/run

.PHONY: all run clean
