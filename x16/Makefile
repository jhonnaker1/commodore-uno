CC65_TARGET = cx16
SRC = src/main.c src/x16vera.c src/x16snd.c src/input.c src/ui.c \
      src/cards.c src/game.c src/ai.c src/vsync.s
OUT = build/uno.prg

# Path to your x16emu install (contains x16emu + rom.bin)
X16EMU ?= /Users/jhonnaker/x16emu_macos_m1-r48

.PHONY: all run clean

all: $(OUT)

$(OUT): $(SRC)
	cl65 -t $(CC65_TARGET) -O -o $(OUT) $(SRC)

run: all
	cd $(X16EMU) && ./x16emu -prg $(CURDIR)/$(OUT) -run

clean:
	rm -f build/*.prg build/*.o src/*.o
