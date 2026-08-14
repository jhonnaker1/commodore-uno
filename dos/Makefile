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

.PHONY: all img run run-xt smoke run-smoke clean

all: build/UNO.EXE

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
img: build/uno.img
build/uno.img: build/UNO.EXE
	dd if=/dev/zero of=$@ bs=512 count=2880 2>/dev/null
	mformat -i $@ -f 1440 ::
	mcopy -i $@ build/UNO.EXE ::UNO.EXE
	@echo "$@: 1.44M floppy image containing UNO.EXE"

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
