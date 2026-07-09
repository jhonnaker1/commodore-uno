CC65_TARGET = cbm510
SRC = src/main.c src/vid510.c src/snd510.c src/input.c src/cards.c src/game.c src/ai.c src/ui.c
OUT = build/uno.prg

.PHONY: all run clean

all: $(OUT)

$(OUT): $(SRC)
	cl65 -t $(CC65_TARGET) -O -o $(OUT) $(SRC)

run: all
	xcbm5x0 $(OUT)

clean:
	rm -f build/*.prg build/*.o
