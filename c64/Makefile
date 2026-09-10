CC65_TARGET = c64
SRC = src/main.c src/vic.c src/sid.c src/input.c src/cards.c src/game.c src/ai.c src/ui.c
OUT = build/uno.prg

.PHONY: all charset run clean

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

run: all
	x64sc $(OUT)

clean:
	rm -f build/*.prg build/*.o build/*.bin
