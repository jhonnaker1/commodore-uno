CC65_TARGET = cx16
SRC = src/main.c src/x16vera.c src/x16snd.c src/input.c src/ui.c \
      src/cards.c src/game.c src/ai.c src/vsync.s
OUT = build/uno.prg

# Bitmap-graphics build: a full playable UNO rendered via the KERNAL GRAPH_*
# framebuffer API (pixel card faces, fanned hand) -- kept alongside the
# text-mode game above. Reuses input/sound/game logic; only the render layer
# differs (ui_bmp.c + vbmp.c + graph.s instead of ui.c + x16vera.c).
BMP_SRC = src/main_game_bmp.c src/ui_bmp.c src/vbmp.c src/graph.s src/vsync.s \
          src/input.c src/x16snd.c src/cards.c src/game.c src/ai.c
BMP_OUT = build/unobmp.prg

# The original static rendering prototype (card faces + fan, no game logic).
DEMO_SRC = src/main_bmp.c src/vbmp.c src/graph.s src/cards.c
DEMO_OUT = build/unobmpdemo.prg

# Path to your x16emu install (contains x16emu + rom.bin)
X16EMU ?= /Users/jhonnaker/x16emu_macos_m1-r48

.PHONY: all bmp bmpdemo run run-bmp clean

all: $(OUT)

$(OUT): $(SRC)
	cl65 -t $(CC65_TARGET) -O -o $(OUT) $(SRC)

bmp: $(BMP_OUT)

$(BMP_OUT): $(BMP_SRC)
	cl65 -t $(CC65_TARGET) -O -o $(BMP_OUT) $(BMP_SRC)

bmpdemo: $(DEMO_OUT)

$(DEMO_OUT): $(DEMO_SRC)
	cl65 -t $(CC65_TARGET) -O -o $(DEMO_OUT) $(DEMO_SRC)

run: all
	cd $(X16EMU) && ./x16emu -prg $(CURDIR)/$(OUT) -run

run-bmp: bmp
	cd $(X16EMU) && ./x16emu -prg $(CURDIR)/$(BMP_OUT) -run

clean:
	rm -f build/*.prg build/*.o src/*.o
