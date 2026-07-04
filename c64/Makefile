CC65_TARGET = c64
SRC = src/main.c src/vic.c src/sid.c src/input.c src/cards.c src/game.c src/ai.c src/ui.c
OUT = build/uno.prg

.PHONY: all charset run clean

# charset_data.h/charset_codes.h are pre-generated and checked in, so a
# fresh clone builds out of the box. Run `make charset` explicitly (and
# edit CHARGEN_PATH in tools/gen_charset.py to point at your own dumped
# C64 chargen ROM) only if you want to regenerate them.
all: $(OUT)

charset:
	python3 tools/gen_charset.py

$(OUT): $(SRC) src/charset_data.h src/charset_codes.h
	cl65 -t $(CC65_TARGET) -O -o $(OUT) $(SRC)

run: all
	x64sc $(OUT)

clean:
	rm -f build/*.prg build/*.o build/*.bin
