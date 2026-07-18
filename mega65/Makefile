CC65_TARGET = c64
SRC = src/main.c src/mega65vid.c src/mega65snd.c src/input.c src/ui.c \
      src/cards.c src/game.c src/ai.c
OUT = build/uno.prg

# mega65-libc provides the VIC-IV conio. Cloned + built on first `make`
# (like the c64os port fetches its assembler). Override MEGA65_LIBC to use
# a copy you already have.
MEGA65_LIBC ?= mega65-libc
LIBC_INC = $(MEGA65_LIBC)/include
LIBC_A = $(MEGA65_LIBC)/libmega65.a

# MEGA65 emulator (LGB Xemu) + ROM, for `make run`.
XMEGA65 ?= /Users/jhonnaker/xemu/bin/xmega65
M65ROM ?= /Users/jhonnaker/xemu/mega65-920395.bin

.PHONY: all run clean

all: $(OUT)

$(MEGA65_LIBC):
	git clone --depth 1 https://github.com/mega65/mega65-libc.git $(MEGA65_LIBC)

$(LIBC_A): | $(MEGA65_LIBC)
	$(MAKE) -C $(MEGA65_LIBC) -f Makefile_cc65 USE_LOCAL_CC65=1 libmega65.a
	# drop random.o: its rand/srand clash with cc65 stdlib
	ar65 d $(LIBC_A) random.o

$(OUT): $(SRC) $(LIBC_A)
	cl65 -t $(CC65_TARGET) -I $(LIBC_INC) -o $(OUT) $(SRC) $(LIBC_A)

run: all
	$(XMEGA65) -rom $(M65ROM) -prg $(CURDIR)/$(OUT)

clean:
	rm -f build/*.prg src/*.o
