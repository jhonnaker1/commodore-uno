# The tms9900 toolchain isn't packaged anywhere -- it's gcc 4.4 + binutils
# 2.19.1 with mburkley's TMS9900 patches, built from source into a per-user
# prefix, same idea as CMOC_DIR/Z88DK_DIR in the coco/ and zxspectrum/ ports.
# See README.md for the build (there are two macOS-specific fixes needed).
TI99_DIR ?= $(HOME)/ti99-toolchain
LIBTI99_DIR ?= $(TI99_DIR)/libti99

CC = $(TI99_DIR)/bin/tms9900-gcc
AS = $(TI99_DIR)/bin/tms9900-as
LD = $(TI99_DIR)/bin/tms9900-ld
ELF2CART = $(TI99_DIR)/bin/elf2cart

MAME ?= $(HOME)/mame0281-arm64/mame
MAMEDIR ?= $(HOME)/mame0281-arm64

# -fno-builtin: there's no libc here, so gcc must not turn loops into calls to
# memcpy/memset that nothing provides.
# -Isrc/compat comes first so <stdlib.h> resolves to our rand()/srand() shim.
CFLAGS = -O2 -std=c99 -fno-builtin -Isrc/compat -I$(LIBTI99_DIR)

# The game links to run from expansion RAM at >A000 (see src/ti99.ld) rather
# than out of the cartridge ROM -- at ~13K it doesn't fit in the 8K the
# cartridge window shows, so the cart is a two-bank loader that copies it into
# RAM first. The stub itself links at the ROM window.
LDFLAGS = -T src/ti99.ld
LOADER_LDFLAGS = --section-start .text=6000

OBJS = build/main.o build/tivid.o build/tisound.o build/tiinput.o \
       build/tirand.o build/cards.o build/game.o build/ai.o build/ui.o

.PHONY: all run clean

all: build/uno.rpk

build:
	mkdir -p build

build/%.o: src/%.c | build
	$(CC) -c $< $(CFLAGS) -o $@

build/%.o: src/%.asm | build
	$(AS) $< -o $@

# libgcc supplies the helpers this CPU has no instructions for (32-bit shifts
# and division). It isn't picked up automatically because we invoke ld
# directly rather than through the gcc driver -- the driver would also drag in
# crt files that assume a hosted, disk-loaded program.
LIBGCC_DIR = $(TI99_DIR)/lib/gcc/tms9900/4.4.0

build/uno.elf: $(OBJS) src/ti99.ld
	$(LD) $(OBJS) $(LDFLAGS) -L$(LIBTI99_DIR) -lti99 \
	  -L$(LIBGCC_DIR) -lgcc -o $@ -Map build/uno.map

build/loader.elf: build/loader.o
	$(LD) build/loader.o $(LOADER_LDFLAGS) -o $@

build/uno.bin: build/loader.elf build/uno.elf
	python3 tools/pack.py build/loader.elf build/uno.elf $@ $(TI99_DIR)/bin/tms9900

build/uno.rpk: build/uno.bin
	python3 tools/mkrpk.py $< $@ paged

# The 32K expansion lives in the peripheral expansion box, so it has to be
# plugged in explicitly; without it there's nowhere for .data/.bss or the stack.
run: build/uno.rpk
	cd $(MAMEDIR) && $(MAME) ti99_4a -cart $(CURDIR)/build/uno.rpk \
	  -ioport peb -ioport:peb:slot2 32kmem \
	  -skip_gameinfo -window -resolution 1024x768

clean:
	rm -rf build
