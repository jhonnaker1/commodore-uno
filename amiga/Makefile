CC = m68k-amigaos-gcc

# Text build (console.device + ANSI colour): build/uno
SRC = src/main.c src/amigacon.c src/amigasound.c src/input.c src/cards.c src/game.c src/ai.c src/ui.c
OUT = build/uno

# Bitmap-graphics build (custom 16-colour screen, pixel-art cards): build/unobmp
# Reuses the shared logic + input.c + sound; swaps the console/UI layer for
# gfx.c/ui_bmp.c and the game loop for main_bmp.c. The text build is untouched.
SRC_BMP = src/main_bmp.c src/gfx.c src/ui_bmp.c src/amigasound.c src/input.c src/cards.c src/game.c src/ai.c
OUT_BMP = build/unobmp

.PHONY: all bmp clean

all: $(OUT)

bmp: $(OUT_BMP)

$(OUT): $(SRC)
	$(CC) -o $(OUT) $(SRC)

$(OUT_BMP): $(SRC_BMP)
	$(CC) -o $(OUT_BMP) $(SRC_BMP)

clean:
	rm -f $(OUT) $(OUT_BMP) build/*.o
