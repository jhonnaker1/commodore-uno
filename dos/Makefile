# Open Watcom is the only actively-maintained compiler that still targets
# 16-bit real-mode DOS. It isn't packaged for macOS, so it's built from
# source and the finished toolchain installed into a per-user prefix, the
# same shape as CMOC_DIR/VBCC/TI99_DIR in the other from-source ports --
# but unlike those it builds natively on Apple Silicon, so the host
# binaries are under armo64/ rather than a bin/. See README.md.
#
# This points at the *installed* toolchain, not at the source tree: the
# build leaves a 2GB checkout behind, of which only the release tree is
# needed to compile anything, so that gets moved out and the checkout can
# be deleted.
WATCOM ?= $(HOME)/dos-toolchain
OWBIN ?= $(WATCOM)/armo64

export WATCOM
export INCLUDE = $(WATCOM)/h
export PATH := $(OWBIN):$(PATH)

WCL = $(OWBIN)/wcl

DOSBOX ?= /Applications/DOSBox-X.app/Contents/MacOS/dosbox-x

# -0   target the 8088/8086 instruction set, so it runs on a stock PC/XT
#      and on everything after it. Nothing here needs a 286.
# -ms  small model: code and data each inside one 64K segment. The game is
#      nowhere near either limit, and it keeps every pointer near except the
#      explicit far one at the video framebuffer.
# -ox  full optimisation, -zq quiet, -bcl=dos build a DOS executable.
CFLAGS = -0 -ms -ox -zq -bcl=dos -zp1

SRC = src/main.c src/ui.c src/cgavid.c src/pcspk.c src/dosinput.c \
      src/cards.c src/game.c src/ai.c

.PHONY: all ega vga img run run-ega run-vga run-xt smoke run-smoke run-smoke-ega run-smoke-vga clean

all: build/UNO.EXE

# ---- EGA build ------------------------------------------------------
# Same 8088 code, a different video backend: 640x350 in 16 colours, which
# is the highest resolution of any port in this repo. ui_bmp.c lays itself
# out from GFX_W/GFX_H so the VGA backend can share it unchanged.
EGA_SRC = src/main_bmp.c src/ui_bmp.c src/egavid.c src/pcspk.c \
          src/dosinput.c src/cards.c src/game.c src/ai.c
EGA_SMOKE_SRC = src/smoke_bmp.c src/ui_bmp.c src/egavid.c \
                src/cards.c src/game.c src/ai.c

ega: build/UNOEGA.EXE
build/UNOEGA.EXE: $(EGA_SRC) | build
	$(WCL) $(CFLAGS) -DGFX_EGA -fo=build/ -fe=$@ $(EGA_SRC)
	@echo "$@: $$(wc -c < $@) bytes"

build/SMOKEEGA.EXE: $(EGA_SMOKE_SRC) | build
	$(WCL) $(CFLAGS) -DGFX_EGA -fo=build/ -fe=$@ $(EGA_SMOKE_SRC)

run-ega: build/UNOEGA.EXE
	$(DOSBOX) -nopromptfolder -machine ega \
	  -c "mount c $(CURDIR)/build" -c "c:" -c "UNOEGA.EXE"

# Renders title + table, dumps the raw planes, and converts them to PNG on
# the host -- a bitmap can't be checked as text the way the CGA build can.
run-smoke-ega: build/SMOKEEGA.EXE
	@rm -f build/TITLE.RAW build/TABLE.RAW
	@$(DOSBOX) -silent -nopromptfolder -machine ega \
	  -c "mount c $(CURDIR)/build" -c "c:" -c "SMOKEEGA.EXE" -c "exit" >/dev/null 2>&1
	@python3 tools/dumptopng.py build/TITLE.RAW build/ega-title.png
	@python3 tools/dumptopng.py build/TABLE.RAW build/ega-table.png

# ---- VGA build ------------------------------------------------------
# The same ui_bmp.c over a completely different backend: 320x200 with one
# byte per pixel and a programmable 256-colour palette. Lower resolution
# than EGA, far simpler hardware, better colours.
VGA_SRC = src/main_bmp.c src/ui_bmp.c src/vgavid.c src/pcspk.c \
          src/dosinput.c src/cards.c src/game.c src/ai.c
VGA_SMOKE_SRC = src/smoke_bmp.c src/ui_bmp.c src/vgavid.c \
                src/cards.c src/game.c src/ai.c

vga: build/UNOVGA.EXE
build/UNOVGA.EXE: $(VGA_SRC) | build
	$(WCL) $(CFLAGS) -DGFX_VGA -fo=build/ -fe=$@ $(VGA_SRC)
	@echo "$@: $$(wc -c < $@) bytes"

build/SMOKEVGA.EXE: $(VGA_SMOKE_SRC) | build
	$(WCL) $(CFLAGS) -DGFX_VGA -fo=build/ -fe=$@ $(VGA_SMOKE_SRC)

run-vga: build/UNOVGA.EXE
	$(DOSBOX) -nopromptfolder -machine vgaonly \
	  -c "mount c $(CURDIR)/build" -c "c:" -c "UNOVGA.EXE"

run-smoke-vga: build/SMOKEVGA.EXE
	@rm -f build/TITLE.RAW build/TABLE.RAW
	@$(DOSBOX) -silent -nopromptfolder -machine vgaonly \
	  -c "mount c $(CURDIR)/build" -c "c:" -c "SMOKEVGA.EXE" -c "exit" >/dev/null 2>&1
	@python3 tools/dumptopng.py build/TITLE.RAW build/vga-title.png
	@python3 tools/dumptopng.py build/TABLE.RAW build/vga-table.png

SMOKE_SRC = src/smoke.c src/ui.c src/cgavid.c src/cards.c src/game.c src/ai.c

smoke: build/SMOKE.EXE
build/SMOKE.EXE: $(SMOKE_SRC) | build
	$(WCL) $(CFLAGS) -fo=build/ -fe=$@ $(SMOKE_SRC)

# Renders the title screen and writes build/SCREEN.TXT for the host to read.
run-smoke: build/SMOKE.EXE
	@rm -f build/SCREEN.TXT
	@$(DOSBOX) -silent -nopromptfolder -machine cga \
	  -c "mount c $(CURDIR)/build" -c "c:" -c "SMOKE.EXE" -c "exit" >/dev/null 2>&1
	@cat build/SCREEN.TXT

build:
	mkdir -p build

build/UNO.EXE: $(SRC) | build
	$(WCL) $(CFLAGS) -fo=build/ -fe=$@ $(SRC)
	@echo "$@: $$(wc -c < $@) bytes"

# A 1.44M FAT12 floppy, which is what the release ships: it drops straight
# into DOSBox, 86Box, PCem or a real drive without any extra tooling.
# All three builds on one floppy: pick by video card at the prompt.
img: build/uno.img
build/uno.img: build/UNO.EXE build/UNOEGA.EXE build/UNOVGA.EXE
	dd if=/dev/zero of=$@ bs=512 count=2880 2>/dev/null
	mformat -i $@ -f 1440 ::
	mcopy -i $@ build/UNO.EXE ::UNO.EXE
	mcopy -i $@ build/UNOEGA.EXE ::UNOEGA.EXE
	mcopy -i $@ build/UNOVGA.EXE ::UNOVGA.EXE
	@echo "$@: 1.44M floppy with UNO.EXE (CGA), UNOEGA.EXE, UNOVGA.EXE"

# Iterate against DOSBox-X emulating CGA. It is not cycle-accurate for an
# XT, but it is scriptable, which 86Box is not -- so correctness gets
# checked here and timing gets checked on a real XT config (run-xt).
run: build/UNO.EXE
	$(DOSBOX) -nopromptfolder -machine cga \
	  -c "mount c $(CURDIR)/build" -c "c:" -c "UNO.EXE"

# Same binary on a 4.77MHz 8088 with genuine CGA timing.
run-xt: build/uno.img
	@echo "Load $(CURDIR)/build/uno.img as floppy A: in 86Box"
	@echo "with machine = IBM XT and video = CGA, then run A:\\UNO.EXE"

clean:
	rm -rf build
