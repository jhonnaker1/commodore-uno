# Foenix F256K build. Needs cc65 (cl65/cc65/ca65/ld65) on PATH. Produces a
# PGZ that FoenixMCP loads off the SD card; run it from SuperBASIC with
# `/- uno`. Toolchain config + startup + f256jr.lib come from the FoenixMCP
# kernel/dos repo (ghackwrench/F256_Jr_Kernel_DOS), vendored in toolchain/.
CPU     = --cpu 65C02
CFG     = toolchain/f256jr_pgz.cfg
LIB     = toolchain/f256jr.lib
CFLAGS  = -g $(CPU) -t none -Os -I src -I toolchain -D_F256JR_

# The full game (shared logic + F256 layers). ui_f256/main/sound added later.
GAME_SRC = main.c f256vid.c ui_f256.c f256snd.c f256input.c cards.c game.c ai.c
GAME_OBJ = $(addprefix build/, pgz.o kernel.o $(GAME_SRC:.c=.o))
GAME_PGZ = build/uno.pgz

SMOKE_OBJ = build/pgz.o build/kernel.o build/smoke.o build/f256vid.o
SMOKE_PGZ = build/uno-smoke.pgz

# macOS `make run`: copies the PGZ onto a working copy of your SD image and
# launches the foenix MAME (then type `/- uno` at the SuperBASIC prompt).
MAME_DIR ?= /Users/jhonnaker/Downloads/mame-foenix256k
SDCARD   ?= $(MAME_DIR)/sdcard.img

.PHONY: all smoke run clean
all: $(GAME_PGZ)
smoke: $(SMOKE_PGZ)

run: all
	@bash run.sh "$(MAME_DIR)" "$(SDCARD)" "$(GAME_PGZ)"

$(GAME_PGZ): $(GAME_OBJ)
	ld65 -C $(CFG) -o $@ $(GAME_OBJ) $(LIB) -m $(@:.pgz=.map)

$(SMOKE_PGZ): $(SMOKE_OBJ)
	ld65 -C $(CFG) -o $@ $(SMOKE_OBJ) $(LIB) -m $(@:.pgz=.map)

build/pgz.o: toolchain/pgz.s
	ca65 -t none $(CPU) $< -o $@

build/kernel.o: toolchain/kernel.c toolchain/api.h
	cc65 $(CFLAGS) -T $< -o build/kernel.s
	ca65 -t none $(CPU) build/kernel.s -o $@

build/%.o: src/%.c
	cc65 $(CFLAGS) -T $< -o build/$*.s
	ca65 -t none $(CPU) build/$*.s -o $@

clean:
	rm -f build/*.o build/*.s build/*.pgz build/*.map
