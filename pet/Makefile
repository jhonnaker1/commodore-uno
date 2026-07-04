CC65_TARGET = pet
SRC = src/main.c src/petvid.c src/petsound.c src/input.c src/cards.c src/game.c src/ai.c src/ui.c
OUT = build/uno.prg

.PHONY: all run clean

all: $(OUT)

$(OUT): $(SRC)
	cl65 -t $(CC65_TARGET) -O -o $(OUT) $(SRC)

run: all
	xpet $(OUT)

clean:
	rm -f build/*.prg build/*.o
