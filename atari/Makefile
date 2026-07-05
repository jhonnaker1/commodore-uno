CC65_TARGET = atarixl
SRC = src/main.c src/atarivid.c src/atarisound.c src/input.c src/cards.c src/game.c src/ai.c src/ui.c
OUT = build/uno.xex

# Building (make/make all) needs only cc65 and doesn't touch this. Running
# in the emulator (make run) needs an Atari XL/XE OS ROM image, which isn't
# included here (copyrighted Atari firmware) -- point XLXE_ROM at your own
# dump, e.g.: make run XLXE_ROM=/path/to/atarixl.rom
XLXE_ROM ?= atarixl.rom

.PHONY: all run clean

all: $(OUT)

$(OUT): $(SRC)
	cl65 -t $(CC65_TARGET) -O -o $(OUT) $(SRC)

run: all
	atari800 -xl -xlxe_rom $(XLXE_ROM) -nobasic $(OUT)

clean:
	rm -f build/*.xex build/*.o
