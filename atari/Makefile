CC65_TARGET = atarixl
SRC = src/main.c src/atarivid.c src/atarisound.c src/input.c src/cards.c src/game.c src/ai.c src/ui.c
OUT = build/uno.xex

SRC_VBXE = src/main_vbxe.c src/vbxevid.c src/atarisound.c src/input.c src/cards.c src/game.c src/ai.c src/ui_vbxe.c
OUT_VBXE = build/unovbxe.xex

# Building (make/make all) needs only cc65 and doesn't touch this. Running
# in the emulator (make run) needs an Atari XL/XE OS ROM image, which isn't
# included here (copyrighted Atari firmware) -- point XLXE_ROM at your own
# dump, e.g.: make run XLXE_ROM=/path/to/atarixl.rom
XLXE_ROM ?= atarixl.rom

# Running the VBXE build (make run-vbxe) needs Altirra (via Wine) with a
# one-time GUI-configured Profile named XL_VBXE that has the Video Board XE
# device added (System > Profile > Edit Profiles, then System > Peripherals
# > Devices) -- Altirra has no command-line switch to add hardware devices,
# only to select a saved profile. Point ALTIRRA_DIR at your install if it's
# not the default below. /gdi forces Altirra's software renderer -- its
# default hardware-accelerated renderer (translated through Wine's
# Vulkan/MoltenVK layer on this Mac) renders a solid color instead of
# the actual display.
ALTIRRA_DIR ?= /Applications/Altirra-4.40

.PHONY: all all-vbxe run run-vbxe clean

all: $(OUT)

all-vbxe: $(OUT_VBXE)

$(OUT): $(SRC)
	cl65 -t $(CC65_TARGET) -O -o $(OUT) $(SRC)

$(OUT_VBXE): $(SRC_VBXE)
	cl65 -t $(CC65_TARGET) -O --start-addr 0x4000 -o $(OUT_VBXE) $(SRC_VBXE)

run: all
	atari800 -xl -xlxe_rom $(XLXE_ROM) -nobasic $(OUT)

run-vbxe: all-vbxe
	wine "$(ALTIRRA_DIR)/Altirra64.exe" /gdi /profile:XL_VBXE "$$(winepath -w '$(CURDIR)/$(OUT_VBXE)')"

clean:
	rm -f build/*.xex build/*.o
