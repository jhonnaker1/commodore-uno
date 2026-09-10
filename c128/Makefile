CC65_TARGET = c128
SRC = src/main.c src/vic.c src/sid.c src/input.c src/cards.c src/game.c src/ai.c src/ui.c
OUT = build/uno128.prg

VDC_SRC = src/main_vdc.c src/vdc.c src/sid.c src/input.c src/cards.c src/game.c src/ai.c src/ui_vdc.c
VDC_OUT = build/uno128vdc.prg

.PHONY: all charset run run-vdc clean

# charset_data.h/charset_codes.h are pre-generated and checked in, so a
# fresh clone builds out of the box. `make charset` regenerates them, and
# needs your own dumped C64 chargen ROM -- it isn't redistributable, so it
# is not in this repo. Point CHARGEN at it:
#   make charset CHARGEN=/path/to/chargen
CHARGEN ?= $(HOME)/Downloads/WinVICE-3.1-x64/C64/chargen

all: $(OUT)

charset:
	CHARGEN="$(CHARGEN)" python3 tools/gen_charset.py

$(OUT): $(SRC) src/charset_data.h src/charset_codes.h
	cl65 -t $(CC65_TARGET) -O -o $(OUT) $(SRC)

# 80-column version using the VDC (8563/8568) instead of the VIC-IIe.
# Reuses the same charset data -- see vdc.c for the VDC-specific quirks
# (character generator memory needs 16-byte-aligned glyph slots even for
# an 8-scanline font, and register 28's char-base bits get re-asserted by
# the KERNAL's 80-column cursor IRQ so have to be rewritten every frame).
$(VDC_OUT): $(VDC_SRC) src/charset_data.h src/charset_codes.h
	cl65 -t $(CC65_TARGET) -O -o $(VDC_OUT) $(VDC_SRC)

run: all
	x128 $(OUT)

run-vdc: $(VDC_OUT)
	x128 $(VDC_OUT)

clean:
	rm -f build/*.prg build/*.o build/*.bin
