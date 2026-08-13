	;; MSX cartridge startup for SDCC.
	;;
	;; SDCC ships crt0 files for CP/M and for bare Z80 boards, but not for
	;; an MSX ROM cartridge, so this is the whole C runtime: the 16-byte
	;; cartridge header the BIOS looks for, a stack, and the two memory
	;; fixups (zero .bss, copy initialisers out of ROM) that C requires and
	;; that nothing else does for us here.
	;;
	;; Link this file FIRST so `init` lands at the start of _CODE, which is
	;; what --code-loc points the header's INIT vector at.

	.module crt0
	.globl	_main

	;; Start/length symbols the linker synthesises for each area once it
	;; knows where everything landed. The assembler has to be told they
	;; are external or it treats them as undefined.
	.globl	s__DATA
	.globl	l__DATA
	.globl	s__INITIALIZED
	.globl	s__INITIALIZER
	.globl	l__INITIALIZER

	;; ---- cartridge header -------------------------------------------
	;; At boot the BIOS walks every slot and subslot looking for the two
	;; bytes "AB" at 0x4000. Finding them, it calls the address in the
	;; INIT word. That is the entire boot protocol -- there is no loader,
	;; no file system and no relocation.
	.area	_HEADER (ABS)
	.org	0x4000
	.db	0x41, 0x42		; "AB" -- ROM signature
	.dw	init			; INIT:      entry point, called at boot
	.dw	0x0000			; STATEMENT: no BASIC CALL extensions
	.dw	0x0000			; DEVICE:    no BASIC device driver
	.dw	0x0000			; TEXT:      no tokenised BASIC in ROM
	.dw	0x0000, 0x0000, 0x0000	; reserved

	.area	_CODE
init::
	;; HIMEM is the top of free RAM as the BIOS worked it out at boot. It
	;; sits below the BIOS work area, and below a disk interface's own
	;; variables if one is installed -- so reading it puts the stack in a
	;; safe place on machines where a hardcoded 0xF380 would be sitting on
	;; top of DiskROM state.
	ld	hl, (0xFC4A)		; HIMEM
	ld	sp, hl

	call	gsinit
	call	_main
	;; main() never returns in this program, but if it ever did there is
	;; nothing sensible to return to -- the BIOS called us from its slot
	;; scan and does not expect control back.
1$:	jr	1$

	;; ---- area ordering ----------------------------------------------
	;; Naming the areas here fixes their order in the output. ROM areas
	;; first (they are placed from --code-loc, inside the cartridge), then
	;; the RAM areas (placed from --data-loc, up in page 3).
	.area	_HOME
	.area	_CODE
	.area	_INITIALIZER
	.area	_GSINIT
	.area	_GSFINAL

	.area	_DATA
	.area	_INITIALIZED
	.area	_BSEG
	.area	_BSS
	.area	_HEAP

	.area	_GSINIT
gsinit::
	;; Zero _DATA. SDCC puts every uninitialised global and static there,
	;; and C says they start at zero. On a disk-loaded program the loader
	;; would have handed us fresh zeroed memory; from ROM we get whatever
	;; the last program left behind, so we have to do it ourselves.
	ld	bc, #l__DATA
	ld	a, b
	or	a, c
	jr	Z, gsinit_init
	ld	hl, #s__DATA
	ld	(hl), #0x00
	dec	bc
	ld	a, b
	or	a, c
	jr	Z, gsinit_init
	ld	de, #s__DATA + 1
	ldir				; spread the first zero over the rest

gsinit_init:
	;; Copy the initialisers. Globals with a value live twice: the values
	;; sit in _INITIALIZER inside the ROM, and the variables themselves in
	;; _INITIALIZED up in RAM. Nothing connects the two but this copy.
	ld	bc, #l__INITIALIZER
	ld	a, b
	or	a, c
	jr	Z, gsinit_done
	ld	de, #s__INITIALIZED
	ld	hl, #s__INITIALIZER
	ldir
gsinit_done:

	.area	_GSFINAL
	ret
