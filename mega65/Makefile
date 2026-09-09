# Two builds of the same game for the same machine:
#
#   make          build/uno.prg        C64 mode, 40 columns  (cc65)
#   make native   build/uno-native.prg native mode, 80 cols  (llvm-mos)
#
# They need different compilers because they run in different modes. The
# C64-mode build is a $0801 cc65 c64-target binary; 80 columns are simply
# not reachable from there, because the C64 never had them. The native
# build links to $2001 with a BASIC 65 header, which is what puts the
# machine in C65/MEGA65 native mode where VIC-IV's H640 is real. cc65 has
# no MEGA65 target at all, so that build uses llvm-mos.

CC65_TARGET = c64
SRC = src/main.c src/mega65vid.c src/mega65snd.c src/mega65spr.c src/input.c src/ui.c \
      src/cards.c src/game.c src/ai.c
OUT = build/uno.prg

NATIVE_SRC = src/main_native.c src/m65native.c src/mega65snd.c src/input_native.c \
             src/ui_native.c src/cards.c src/game.c src/ai.c
NATIVE_OUT = build/uno-native.prg

# mega65-libc provides the VIC-IV conio. Cloned + built on first `make`
# (like the c64os port fetches its assembler). Override MEGA65_LIBC to use
# a copy you already have.
MEGA65_LIBC ?= mega65-libc
LIBC_INC = $(MEGA65_LIBC)/include
LIBC_A = $(MEGA65_LIBC)/libmega65.a
# The same library built the other way. mega65-libc supports both toolchains
# (Makefile_cc65 vs its CMake path), but the two artifacts are not
# interchangeable, so each build gets its own tree.
MEGA65_LIBC_LLVM ?= mega65-libc-llvm
LIBC_LLVM_INC = $(MEGA65_LIBC_LLVM)/include
LIBC_LLVM_A = $(MEGA65_LIBC_LLVM)/build/src/libmega65libc.a

# llvm-mos SDK (prebuilt; https://github.com/llvm-mos/llvm-mos-sdk/releases).
# Point it at wherever you unpacked it: make native LLVM_MOS=/path/to/llvm-mos
LLVM_MOS ?= $(HOME)/llvm-mos
M65CC = $(LLVM_MOS)/bin/mos-mega65-clang

# MEGA65 emulator (LGB Xemu) + ROM, for `make run`. Supply your own ROM.
XMEGA65 ?= $(HOME)/xemu/bin/xmega65
M65ROM ?= $(HOME)/xemu/mega65-920395.bin

.PHONY: all native run run-native clean

all: $(OUT)

$(MEGA65_LIBC):
	git clone --depth 1 https://github.com/mega65/mega65-libc.git $(MEGA65_LIBC)

$(LIBC_A): | $(MEGA65_LIBC)
	$(MAKE) -C $(MEGA65_LIBC) -f Makefile_cc65 USE_LOCAL_CC65=1 libmega65.a
	# drop random.o: its rand/srand clash with cc65 stdlib
	ar65 d $(LIBC_A) random.o

$(OUT): $(SRC) $(LIBC_A)
	cl65 -t $(CC65_TARGET) -I $(LIBC_INC) -o $(OUT) $(SRC) $(LIBC_A)

native: $(NATIVE_OUT)

$(MEGA65_LIBC_LLVM):
	git clone --depth 1 https://github.com/mega65/mega65-libc.git $(MEGA65_LIBC_LLVM)

# No `ar65 d random.o` equivalent is needed here: llvm-mos's own libc does
# not collide with mega65-libc's rand/srand the way cc65's does.
$(LIBC_LLVM_A): | $(MEGA65_LIBC_LLVM)
	$(MAKE) -C $(MEGA65_LIBC_LLVM) llvm LLVM_MOS_PATH=$(LLVM_MOS)

$(NATIVE_OUT): $(NATIVE_SRC) $(LIBC_LLVM_A)
	$(M65CC) -Wall -Os -DM65_NATIVE -I $(LIBC_LLVM_INC) \
	    -o $(NATIVE_OUT) $(NATIVE_SRC) $(LIBC_LLVM_A)

run: all
	$(XMEGA65) -rom $(M65ROM) -prg $(CURDIR)/$(OUT)

# -prgmode 65 is not strictly required -- Xemu auto-detects C65 mode from
# the $2001 load address -- but stating it makes the intent explicit.
run-native: native
	$(XMEGA65) -rom $(M65ROM) -prgmode 65 -prg $(CURDIR)/$(NATIVE_OUT)

clean:
	rm -f build/*.prg src/*.o
