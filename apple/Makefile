CC65_TARGET = apple2enh
CFG = apple2enh-system.cfg
SRC = src/main.c src/applevid.c src/applesound.c src/input.c src/cards.c src/game.c src/ai.c src/ui.c
OUT = build/uno.system

.PHONY: all clean

all: $(OUT)

# -D,__EXEHDR__=0 disables the AppleSingle-style relocation header that
# apple2enh-system.cfg otherwise prepends. That header is meant to be parsed
# by a smart loader (cc65's LOADER.SYSTEM); a plain "-NAME" ProDOS system
# launch just loads the raw file to $2000 and jumps there, so with the
# header present it jumps straight into header bytes instead of code.
$(OUT): $(SRC)
	cl65 -t $(CC65_TARGET) -C $(CFG) -Wl -D,__EXEHDR__=0 -O -o $(OUT) $(SRC)

clean:
	rm -f build/*.system build/*.o
