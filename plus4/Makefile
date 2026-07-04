CC65_TARGET = plus4
SRC = src/main.c src/ted.c src/tsound.c src/input.c src/cards.c src/game.c src/ai.c src/ui.c
OUT = build/uno4.prg

.PHONY: all run clean

all: $(OUT)

$(OUT): $(SRC)
	cl65 -t $(CC65_TARGET) -O -o $(OUT) $(SRC)

run: all
	xplus4 $(OUT)

clean:
	rm -f build/*.prg build/*.o
