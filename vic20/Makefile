CC65_TARGET = vic20
CFG = vic20-32k.cfg
SRC = src/main.c src/vic20io.c src/vsound.c src/input.c src/cards.c src/game.c src/ai.c src/ui.c
OUT = build/uno20.prg

.PHONY: all run clean

all: $(OUT)

$(OUT): $(SRC)
	cl65 -t $(CC65_TARGET) -C $(CFG) -O -o $(OUT) $(SRC)

run: all
	xvic -memory all $(OUT)

clean:
	rm -f build/*.prg build/*.o
