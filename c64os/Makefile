APP_NAME := uno
APP_VERSION := 1.0
APP_FULLNAME := $(APP_NAME)_$(APP_VERSION)
APP_AUTHOR := Commodore UNO

BINDIR := ./bin
VENVDIR := ./.env
TOOLDIR := ./tools
ROMDIR := ./rom
DLDIR := ./dl
SRCDIR := ./src
INCDIR := ./inc
OUTDIR := ./out
DISTDIR := ./dist

OBJ := $(OUTDIR)/main.o $(OUTDIR)/menu.m $(OUTDIR)/about.t

PYTHON := python3

ASM := $(BINDIR)/tmpx
ASMFLAGS := -I $(INCDIR)

TMPX_VERSION = TMPx_v1.1.0-STYLE
# TMPx ships a linux/win/solaris/freebsd build plus an Intel macOS build
# (maxosx-x86_64 -- note the upstream typo). No native arm64 build exists,
# but the x86_64 one runs fine under Rosetta 2 on Apple Silicon.
TMPX_ARCH = maxosx-x86_64

C64EMU := x64sc
C1541EMU := c1541

C64OS_DHD := $(ROMDIR)/c64os.dhd
CMDHD_ROM := $(ROMDIR)/cmd_hd_bootrom.bin
JD_KERNAL_ROM := $(ROMDIR)/JiffyDOS_C64.bin

C64EMU_EXTRAFLAGS := -controlport1device 3 -fslongnames
C64EMU_ROMFLAGS := -kernal $(JD_KERNAL_ROM) -dosCMDHD $(CMDHD_ROM)
# fs8 must point at the app bundle directory itself (about.t/main.o/menu.m
# at the top level) -- pointing it at $(DISTDIR) instead exposed an extra
# directory level plus the unrelated .d64 file sitting alongside it, which
# confused C64 OS's own file browser when mounting device 8.
# CMD HD is device 10, not 9 here -- 9 had trouble mounting in testing.
C64EMU_MOUNTFLAGS := -busdevice8 -drive8type 0 -drive10type 4844 -fs8 $(DISTDIR)/$(APP_FULLNAME) -10 $(OUTDIR)/c64os.dhd
C64EMU_BOOTCMD := load \"c64os\",10\nrun\n

.PHONY: all dist d64 clean run help

all: dist

$(C64OS_DHD):
	@echo "Missing $(C64OS_DHD) -- copy your own C64 OS disk image here. See README.md."
	@false

$(CMDHD_ROM):
	@echo "Missing $(CMDHD_ROM) -- copy the CMD HD Boot ROM here. See README.md."
	@false

$(VENVDIR):
	$(PYTHON) -m venv $(VENVDIR)
	$(VENVDIR)/bin/pip install -r ./requirements.txt

$(DLDIR)/$(TMPX_VERSION):
	mkdir -p $(DLDIR)
	curl -sL https://style64.org/file/$(TMPX_VERSION).zip -o $(DLDIR)/$(TMPX_VERSION).zip
	unzip -q -o $(DLDIR)/$(TMPX_VERSION).zip -d $(DLDIR)/$(TMPX_VERSION)

$(ASM): $(DLDIR)/$(TMPX_VERSION)
	mkdir -p $(BINDIR)
	cp $</$(TMPX_VERSION)/$(TMPX_ARCH)/tmpx $@
	chmod +x $@

$(OUTDIR):
	mkdir -p $@

# main.o pulls in engine.s via .include, which make can't discover on its
# own (no auto-dependency generation for TMP assembly) -- list it
# explicitly so editing engine.s alone still triggers a rebuild.
$(OUTDIR)/main.o: $(SRCDIR)/main.s $(SRCDIR)/engine.s $(ASM) | $(OUTDIR)
	$(ASM) $(ASMFLAGS) -i $< -o $@

$(OUTDIR)/%.o: $(SRCDIR)/%.s $(ASM) | $(OUTDIR)
	$(ASM) $(ASMFLAGS) -i $< -o $@

$(OUTDIR)/about.t: $(VENVDIR) | $(OUTDIR)
	$(VENVDIR)/bin/python $(TOOLDIR)/gen_meta.py -a "$(APP_AUTHOR)" "$(APP_NAME)" "$(APP_VERSION)" > $@

$(OUTDIR)/menu.m: $(SRCDIR)/menu.json $(VENVDIR) | $(OUTDIR)
	$(VENVDIR)/bin/python $(TOOLDIR)/gen_menu.py $< > $@

$(DISTDIR)/$(APP_FULLNAME): $(OBJ)
	mkdir -p $@
	cp $? $@/

dist: $(DISTDIR)/$(APP_FULLNAME)

$(OUTDIR)/c64os.dhd: $(C64OS_DHD) | $(OUTDIR)
	cp $< $@

$(DISTDIR)/$(APP_FULLNAME).d64: $(DISTDIR)/$(APP_FULLNAME)
	$(C1541EMU) -format $(APP_NAME),8 d64 $@ \
	  -attach $@ \
	  -write $(DISTDIR)/$(APP_FULLNAME)/about.t about.t \
	  -write $(DISTDIR)/$(APP_FULLNAME)/main.o main.o \
	  -write $(DISTDIR)/$(APP_FULLNAME)/menu.m menu.m

d64: $(DISTDIR)/$(APP_FULLNAME).d64

clean:
	rm -rf $(OUTDIR) $(DISTDIR)

run: d64 $(OUTDIR)/c64os.dhd $(CMDHD_ROM)
	$(C64EMU) -default \
	  $(C64EMU_ROMFLAGS) $(C64EMU_EXTRAFLAGS) \
	  $(C64EMU_MOUNTFLAGS) -keybuf "$(C64EMU_BOOTCMD)"

help:
	@echo "supported targets:"
	@echo "  dist:  build re-distributable artifacts (default)"
	@echo "  d64:   create a .d64 disk image (requires VICE's c1541)"
	@echo "  run:   run in emulation (requires VICE's x64sc + rom/ files, see README.md)"
	@echo "  clean: delete all objects and artifacts"
