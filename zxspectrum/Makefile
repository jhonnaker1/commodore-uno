Z88DK_DIR ?= $(HOME)/z88dk
ZCC = $(Z88DK_DIR)/bin/zcc
export ZCCCFG = $(Z88DK_DIR)/lib/config
export PATH := $(Z88DK_DIR)/bin:$(PATH)

SRC = src/main.c src/zxvid.c src/zxsound.c src/zxinput.c src/cards.c src/game.c src/ai.c src/ui.c
OUT = build/uno

MAME ?= mame
SPECTRUM_ROM ?= $(HOME)/mame0281-arm64/roms/spectrum/spectrum.rom

.PHONY: all run clean

all: $(OUT).sna

build:
	mkdir -p build

$(OUT).sna: $(SRC) | build
	$(ZCC) +zx -vn -startup=1 -clib=sdcc_iy $(SRC) -o $(OUT) -create-app -Cz--sna

run: $(OUT).sna
	$(MAME) spectrum -snapshot $(OUT).sna -window -nomax -skip_gameinfo

clean:
	rm -f build/uno* build/*.bin build/*.o
