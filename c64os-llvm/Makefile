# UNO for C64 OS, in C, built by llvm-mos.
#
#   make           build/uno/ -- the app bundle: main.o, menu.m, about.t
#   make d64       build/uno-c64os-llvm.d64, the bundle on a disk image
#   make car       build/unoc.car -- the bundle as C64 OS apps ship: copied
#                  to //os/applications and double-clicked, C64 OS's own
#                  extractor makes the unoc folder
#   make zip       build/uno-c64os-llvm.zip, the release asset: unoc.car
#                  and README.txt in an uno-c64os-llvm/ folder
#   make install   copy the bundle onto a C64 OS disk, read it back, and
#                  print the checksums it found beside the ones expected
#   make run       boot C64 OS from that disk (double-click "unoc" to play)
#   make install-car / run-car   the same, but with only unoc.car, on a
#                  separate fresh disk -- the release, tested the way a
#                  player installs it
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
CAR      = build/unoc.car
ZIP      = build/uno-c64os-llvm.zip

# The archive's date, fixed so the same bundle makes the same .car.
CAR_DATE = 2026-09-29

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

.PHONY: all d64 car zip sums install run install-car run-car clean

all: $(BUNDLE)

$(OBJ): $(SRC) src/cards.h src/game.h src/ai.h src/sid.h c64os.ld
	$(CC) $(CFLAGS) -T c64os.ld -Wl,-Map=build/main.map -o $@ $(SRC)

$(MENU): src/menu.json tools/c64os_meta.py
	python3 tools/c64os_meta.py menu $< > $@

$(ABOUT): tools/c64os_meta.py
	python3 tools/c64os_meta.py about uno 1.0 2026 "Commodore UNO" > $@

# files.txt lists the bundle as "name,type" for mkcar.py: main.o is a PRG,
# menu.m and about.t are SEQ, as in every app C64 OS ships.
$(BUNDLE): $(OBJ) $(MENU) $(ABOUT)
	mkdir -p $@
	cp $^ $@/
	printf 'main.o,p\nmenu.m,s\nabout.t,s\n' > $@/files.txt
	@touch $@

d64: $(D64)

# menu.m and about.t are SEQ, as in every app C64 OS ships; ",s" is how
# c1541 is told.
$(D64): $(BUNDLE)
	rm -f $@
	c1541 -format uno,8 d64 $@ \
	  -write $(BUNDLE)/main.o main.o \
	  -write $(BUNDLE)/menu.m "menu.m,s" \
	  -write $(BUNDLE)/about.t "about.t,s"

car: $(CAR)

# Written by tools/mkcar.py (the ega trek project's, where the format was
# measured against real third-party C64 OS apps) and read straight back --
# every record and the CRC32 trailer -- before anything uses it. The name is
# short and lowercase: it is copied onto a C64 filesystem, 16 characters.
$(CAR): $(BUNDLE) tools/mkcar.py
	python3 tools/mkcar.py write $(BUNDLE) "$(APP)" "UNO for C64 OS, llvm-mos" $(CAR_DATE) $@
	python3 tools/mkcar.py read $@ > /dev/null

zip: $(ZIP)

# The release: unoc.car and README.txt in an uno-c64os-llvm/ folder. Every
# entry carries one fixed timestamp, so the zip reproduces byte for byte --
# its embedded dates are otherwise the one thing that differs between two
# builds of identical files.
$(ZIP): $(CAR) README-release.txt
	python3 -c 'import sys, zipfile; \
	  z = zipfile.ZipFile(sys.argv[1], "w", zipfile.ZIP_DEFLATED); \
	  d = (2026, 9, 29, 0, 0, 0); f = "uno-c64os-llvm/"; \
	  z.writestr(zipfile.ZipInfo(f, d), ""); \
	  [z.writestr(zipfile.ZipInfo(f + n, d), open(s, "rb").read(), zipfile.ZIP_DEFLATED) \
	   for n, s in (("unoc.car", "$(CAR)"), ("README.txt", "README-release.txt"))]; \
	  z.close()' $@

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
	cp $(BUNDLE)/main.o $(BUNDLE)/menu.m $(BUNDLE)/about.t $@/
	sed -e 's/{APP}/$(APP)/g' -e 's/{N}/3/' -e 's/{DATA}/main.o,p,menu.m,s,about.t,s/' \
	  tools/install.bas > $@/install.txt
	petcat -w2 -o $@/install -- $@/install.txt
	rm $@/install.txt
	@touch $@

# The .car test: only unoc.car, into //os/applications itself, on a disk
# of its own -- a fresh C64 OS with no unoc folder, as a player has.
CAR_FS8 = $(CURDIR)/build/fs8car
CAR_DHD = $(CURDIR)/build/cartest.dhd

$(CAR_FS8): $(CAR) tools/install.bas
	mkdir -p $@
	cp $(CAR) $@/
	sed -e 's/{APP}//' -e 's/{N}/1/' -e 's/{DATA}/unoc.car,p/' \
	  tools/install.bas > $@/install.txt
	petcat -w2 -o $@/install -- $@/install.txt
	rm $@/install.txt
	@touch $@

$(CAR_DHD): | $(C64OS_DHD)
	cp $(C64OS_DHD) $@

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

X64_CAR = $(subst -fs8 $(FS8) -10 $(WORK_DHD),-fs8 $(CAR_FS8) -10 $(CAR_DHD),$(X64))

# Prints "unoc.car ok <length>"; close VICE, then `make run-car`, and in
# C64 OS double-click unoc.car in //os/applications, then the unoc folder
# it extracts.
install-car: $(CAR_FS8) $(CAR_DHD) $(CMDHD_ROM) $(JD_KERNAL)
	$(X64_CAR) -warp -keybuf 'load"install",8\nrun\n'

run-car: $(CAR_FS8) $(CAR_DHD) $(CMDHD_ROM) $(JD_KERNAL)
	$(X64_CAR) -keybuf 'load"c64os",10\nrun\n'

clean:
	rm -rf build/main.o build/main.o.elf build/main.map build/menu.m \
	  build/about.t $(BUNDLE) $(FS8) $(CAR_FS8) $(D64) $(CAR) $(ZIP)
