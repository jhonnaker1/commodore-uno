CC65_TARGET = vic20
CFG = vic20-highmem.cfg
SRC = src/main.c src/vic20io.c src/vsound.c src/input.c src/cards.c src/game.c src/ai.c src/ui.c
OUT = build/uno20.prg

# vic20-highmem.cfg's MAIN segment (real code/data) starts here -- must
# match the "start" value in that file. cc65's auto-generated "10 SYS nnnn"
# BASIC stub assumes a contiguous layout and gets this wrong for our
# custom split memory map (see tools/patch_sys_addr.py), so it has to be
# patched in after linking.
MAIN_START = 8193

.PHONY: all run clean

all: $(OUT)

$(OUT): $(SRC)
	cl65 -t $(CC65_TARGET) -C $(CFG) -O -o $(OUT) $(SRC)
	python3 tools/patch_sys_addr.py $(OUT) $(MAIN_START)

run: all
	xvic -memory all $(OUT)

clean:
	rm -f build/*.prg build/*.o
