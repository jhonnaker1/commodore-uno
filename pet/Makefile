CC65_TARGET = pet
SRC = src/main.c src/petvid.c src/petsound.c src/input.c src/cards.c src/game.c src/ai.c src/ui.c
OUT = build/uno.prg

# PET 8032: same hardware family (monochrome, screen at $8000), just
# 80x25 instead of 40x25. Separate build target, not a replacement --
# see petvid8032.h/ui8032.c.
SRC_8032 = src/main8032.c src/petvid8032.c src/petsound.c src/input.c src/cards.c src/game.c src/ai.c src/ui8032.c
OUT_8032 = build/uno8032.prg

.PHONY: all run run-8032 clean

all: $(OUT)

$(OUT): $(SRC)
	cl65 -t $(CC65_TARGET) -O -o $(OUT) $(SRC)

$(OUT_8032): $(SRC_8032)
	cl65 -t $(CC65_TARGET) -O -o $(OUT_8032) $(SRC_8032)

run: all
	xpet $(OUT)

run-8032: $(OUT_8032)
	xpet -model 8032 $(OUT_8032)

clean:
	rm -f build/*.prg build/*.o
