# UNO for C64 OS, in C, built by llvm-mos.
#
#   make           build/uno/ -- the app bundle: main.o, menu.m, about.t
#   make d64       build/uno-c64os-llvm.d64, the bundle on a disk image
#   make install   copy the bundle onto a C64 OS disk, read it back, and
#                  print the checksums it found beside the ones expected
#   make run       boot C64 OS from that disk (double-click "unoc" to play)
#
# The game rules are the shared cards.c/game.c/ai.c -- byte-identical to
# every other C port, which `make check` at the repo root enforces. The
# assembly port in ../c64os is the same application written the other way.

APP      = unoc
BUNDLE   = build/uno
OBJ      = build/main.o
MENU     = build/menu.m
ABOUT    = build/about.t
D64      = build/uno-c64os-llvm.d64

SRC = src/app.s src/main.c src/sid.c src/cards.c src/game.c src/ai.c

# llvm-mos SDK (https://github.com/llvm-mos/llvm-mos-sdk/releases).
#   make LLVM_MOS=/path/to/llvm-mos
LLVM_MOS ?= $(HOME)/llvm-mos
CC       = $(LLVM_MOS)/bin/mos-common-clang
CFLAGS   = -mcpu=mos6502 -Oz -Wall -nostartfiles

# C64 OS is commercial and so are these; `make run`/`make install` need
# your own. The assembly port keeps them in ../c64os/rom -- see its README.
#   make run C64OS_ROMS=/path/to/dir
C64OS_ROMS ?= ../c64os/rom
C64OS_DHD  = $(C64OS_ROMS)/c64os.dhd
CMDHD_ROM  = $(C64OS_ROMS)/cmd_hd_bootrom.bin
JD_KERNAL  = $(C64OS_ROMS)/JiffyDOS_C64.bin

# The disk this port installs onto: a copy, so the original is never
# written. Delete it to start again from a clean C64 OS. Both paths are
# absolute: they are emulator state, not build output, and the repo-wide
# build sweep (tools/build_all.py) builds every `build/...` it finds.
WORK_DHD = $(CURDIR)/build/c64os.dhd
FS8      = $(CURDIR)/build/fs8

# +saveres: VICE otherwise writes these options back into your vicerc.
X64 = x64sc -default +saveres \
      -kernal $(JD_KERNAL) -dosCMDHD $(CMDHD_ROM) \
      -controlport1device 3 -fslongnames -reu -reusize 16384 -keybuf-delay 50 \
      -busdevice8 -drive8type 0 -drive10type 4844 -fs8 $(FS8) -10 $(WORK_DHD)

.PHONY: all d64 sums install run clean

all: $(BUNDLE)

$(OBJ): $(SRC) src/cards.h src/game.h src/ai.h src/sid.h c64os.ld
	$(CC) $(CFLAGS) -T c64os.ld -Wl,-Map=build/main.map -o $@ $(SRC)

$(MENU): src/menu.json tools/c64os_meta.py
	python3 tools/c64os_meta.py menu $< > $@

$(ABOUT): tools/c64os_meta.py
	python3 tools/c64os_meta.py about uno 1.0 2026 "Commodore UNO" > $@

$(BUNDLE): $(OBJ) $(MENU) $(ABOUT)
	mkdir -p $@
	cp $^ $@/
	@touch $@

d64: $(D64)

$(D64): $(BUNDLE)
	rm -f $@
	c1541 -format uno,8 d64 $@ \
	  -write $(BUNDLE)/main.o main.o \
	  -write $(BUNDLE)/menu.m menu.m \
	  -write $(BUNDLE)/about.t about.t

$(C64OS_DHD) $(CMDHD_ROM) $(JD_KERNAL):
	@echo "Missing $@ -- supply your own C64 OS image and ROMs; see README.md."
	@false

$(WORK_DHD): | $(C64OS_DHD)
	cp $(C64OS_DHD) $@

# Device 8 is a host directory holding the bundle and the installer. No
# .prg extension: with -fslongnames the extension is part of the name, and
# `load"install",8` does not find install.prg.
$(FS8): $(BUNDLE) tools/install.bas
	mkdir -p $@
	cp $(BUNDLE)/* $@/
	sed 's/{APP}/$(APP)/g' tools/install.bas > $@/install.txt
	petcat -w2 -o $@/install -- $@/install.txt
	rm $@/install.txt
	@touch $@

# What the installer should print, to compare with the screen.
sums: $(BUNDLE)
	@for f in main.o menu.m about.t; do \
	  python3 -c "import sys; d=open('$(BUNDLE)/'+sys.argv[1],'rb').read(); \
	  print('%-8s %6d %6d' % (sys.argv[1], len(d), sum(d) % 65536))" $$f; done

# BASIC copies a byte per GET#, so an 11K install takes minutes at real
# speed; -warp makes it seconds. Compare the three lines it prints with
# the ones `make sums` printed first, then close VICE.
install: $(FS8) $(WORK_DHD) $(CMDHD_ROM) $(JD_KERNAL) sums
	$(X64) -warp -keybuf 'load"install",8\nrun\n'

run: $(FS8) $(WORK_DHD) $(CMDHD_ROM) $(JD_KERNAL)
	$(X64) -keybuf 'load"c64os",10\nrun\n'

clean:
	rm -rf build/main.o build/main.o.elf build/main.map build/menu.m \
	  build/about.t $(BUNDLE) $(FS8) $(D64)
