# Atari ST/STE build. Needs the vbcc m68k-atari cross-toolchain (vbcc +
# vasm + vlink with the `tos` config); there is no Homebrew package, so see
# README.md for how it was built. Point VBCC at your install:
#   make VBCC=/path/to/vbcc
VBCC ?= /Users/jhonnaker/vbcc
# vc shells out to vbccm68k/vasmm68k_mot/vlink by name, so they must be on PATH
VC = VBCC=$(VBCC) PATH=$(VBCC)/bin:$$PATH $(VBCC)/bin/vc +tos
CFLAGS = -O=1023 -DNDEBUG

SRC = src/main.c src/stevid.c src/stesound.c src/steinput.c src/ui_ste.c \
      src/cards.c src/game.c src/ai.c
OUT = build/UNO.TOS

SRC_SMOKE = src/stevid_smoke.c src/stevid.c
OUT_SMOKE = build/SMOKE.TOS

# Hatari needs a TOS/EmuTOS ROM image; point TOS_ROM at your own.
TOS_ROM ?= /Users/jhonnaker/hatari/rom/etos512us.img
HD = build/hd

.PHONY: all smoke run run-smoke clean

all: $(OUT)

smoke: $(OUT_SMOKE)

$(OUT): $(SRC)
	$(VC) $(CFLAGS) -o $(OUT) $(SRC)

$(OUT_SMOKE): $(SRC_SMOKE)
	$(VC) $(CFLAGS) -o $(OUT_SMOKE) $(SRC_SMOKE)

# Hatari autostarts the program off an emulated GEMDOS drive.
run: all
	@mkdir -p $(HD) && cp $(OUT) $(HD)/
	hatari --machine ste --tos $(TOS_ROM) --harddrive $(HD) \
	       --auto 'C:\UNO.TOS' --confirm-quit false

run-smoke: smoke
	@mkdir -p $(HD) && cp $(OUT_SMOKE) $(HD)/
	hatari --machine ste --tos $(TOS_ROM) --harddrive $(HD) \
	       --auto 'C:\SMOKE.TOS' --confirm-quit false

clean:
	rm -f build/*.TOS build/*.o
	rm -rf $(HD)
