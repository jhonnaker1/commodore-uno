# SDCC is the mainstream MSX cross-compiler and installs straight from
# Homebrew (`brew install sdcc`) -- unlike the CMOC/vbcc/tms9900-gcc
# toolchains the other ports need, there is nothing to build from source.
SDCC ?= sdcc
SDASZ80 ?= sdasz80
MAKEBIN ?= makebin

OPENMSX ?= /Applications/openMSX.app/Contents/MacOS/openmsx
# A stock European MSX2: V9938, 128K VRAM, 128K RAM. The ROM runs on any
# MSX2 (and on MSX2+/turbo R, which are supersets); this is just the machine
# `make run` picks.
MACHINE ?= Philips_NMS_8250

# The cartridge occupies page 1, so code starts just past the 16-byte ROM
# header at 0x4000. Variables and the stack go to page 3, which is RAM on
# every MSX -- page 2 is not guaranteed without slot switching.
CODE_LOC = 0x4010
DATA_LOC = 0xC000

CFLAGS = -mz80 --std-c99 --opt-code-speed -Isrc
LDFLAGS = -mz80 --no-std-crt0 --code-loc $(CODE_LOC) --data-loc $(DATA_LOC)

GAME_SRC = main.c ui.c msxvdp.c msxsnd.c msxinput.c cards.c game.c ai.c
GAME_OBJ = $(addprefix build/,$(GAME_SRC:.c=.rel))
ASM_OBJ = build/crt0.rel build/msxbios.rel

.PHONY: all run smoke run-smoke clean

all: build/uno.rom

build:
	mkdir -p build

build/%.rel: src/%.c | build
	$(SDCC) $(CFLAGS) -c $< -o $@

build/%.rel: src/%.s | build
	$(SDASZ80) -o $@ $<

# crt0.rel must come first on the link line: it is what puts `init` at the
# start of _CODE, which is the address the cartridge header vectors to.
build/uno.ihx: $(ASM_OBJ) $(GAME_OBJ)
	$(SDCC) $(LDFLAGS) $(ASM_OBJ) $(GAME_OBJ) -o $@

build/smoke.ihx: $(ASM_OBJ) build/smoke.rel
	$(SDCC) $(LDFLAGS) $(ASM_OBJ) build/smoke.rel -o $@

# makebin lays the image out from 0x0000, so the cartridge is the second
# 16K of it -- everything below 0x4000 is the hole where the BIOS lives.
build/%.rom: build/%.ihx
	$(MAKEBIN) -s 32768 $< $@.tmp
	dd if=$@.tmp of=$@ bs=1024 skip=16 count=16 2>/dev/null
	rm -f $@.tmp
	@echo "$@: $$(wc -c < $@) bytes"

smoke: build/smoke.rom

run: build/uno.rom
	$(OPENMSX) -machine $(MACHINE) -carta $(CURDIR)/build/uno.rom

run-smoke: build/smoke.rom
	$(OPENMSX) -machine $(MACHINE) -carta $(CURDIR)/build/smoke.rom

clean:
	rm -rf build
