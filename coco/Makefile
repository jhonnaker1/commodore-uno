# CMOC isn't brew-installable -- it's built from source (see README.md)
# and installed to a per-user prefix, same idea as Z88DK_DIR in the
# zxspectrum/ port's Makefile.
CMOC_DIR ?= $(HOME)/cmoc
CMOC = $(CMOC_DIR)/bin/cmoc
# The cmoc binary has /usr/local/share/cmoc baked in as its default
# PKGDATADIR unless overridden -- exporting it here means CMOC_DIR can
# actually differ from that default without a rebuild of cmoc itself.
export PKGDATADIR = $(CMOC_DIR)/share/cmoc

SRC = src/main.c src/cocovid.c src/cocosound.c src/cocoinput.c src/cards.c src/game.c src/ai.c src/ui.c
OUT = build/uno.bin

XROAR ?= xroar
ROMDIR = rom
COCO3_ROM = $(ROMDIR)/coco3.rom

.PHONY: all run clean

all: $(OUT)

build:
	mkdir -p build

$(OUT): $(SRC) | build
	$(CMOC) --dos -Isrc/compat -o $(OUT) $(SRC)

$(COCO3_ROM):
	@echo "Missing $(COCO3_ROM) -- copy your own CoCo 3 ROM here (32KB combined Color/Extended/Super Extended BASIC). See README.md."
	@false

# -run: XRoar's own autorun -- attaches the .bin and types LOADM/EXEC
# for us, no manual disk image needed.
run: $(OUT) $(COCO3_ROM)
	$(XROAR) -rompath $(ROMDIR) -machine coco3 -run $(OUT)

clean:
	rm -f build/uno.bin build/*.o
