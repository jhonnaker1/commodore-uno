CC = m68k-amigaos-gcc
SRC = src/main.c src/amigacon.c src/amigasound.c src/input.c src/cards.c src/game.c src/ai.c src/ui.c
OUT = build/uno

.PHONY: all clean

all: $(OUT)

$(OUT): $(SRC)
	$(CC) -o $(OUT) $(SRC)

clean:
	rm -f $(OUT) build/*.o
