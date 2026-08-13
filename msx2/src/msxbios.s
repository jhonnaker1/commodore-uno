	;; C-callable wrappers around the handful of MSX BIOS entry points this
	;; port uses.
	;;
	;; Everything performance-critical (VRAM, the blitter, the PSG) talks to
	;; the hardware directly through __sfr ports in C -- see msxvdp.c. The
	;; BIOS is used only where it earns its keep: screen mode setup, which
	;; has to be right across every MSX2 variant, and the keyboard/joystick
	;; readers, which the BIOS already debounces in its own interrupt
	;; handler every frame.
	;;
	;; Our cartridge lives in page 1 (0x4000-0x7FFF); page 0 is still the
	;; BIOS ROM, so these are plain calls with no slot switching.
	;;
	;; SDCC 4.x's default convention (--sdcccall 1) hands the first char
	;; argument in A and returns a char in A, which is exactly what these
	;; BIOS routines already want -- so most wrappers are just the call.
	;; IX is the one register SDCC expects a callee to preserve, and BIOS
	;; routines are free to trash it.

	.module msxbios
	.area	_CODE

	;; CHGMOD -- initialise the screen to the mode in A (5 = 256x212, 16
	;; colours). Doing this through the BIOS rather than by poking mode
	;; registers means the VRAM table bases and the machine-specific bits
	;; come out right without us hand-computing R#2/R#3/R#4/R#5/R#10.
_bios_chgmod::
	push	ix
	call	0x005F
	pop	ix
	ret

	;; GTSTCK -- direction of stick A (0 = the cursor keys, 1/2 = the two
	;; joystick ports). Returns 0 for centred, 1-8 clockwise from up.
_bios_gtstck::
	push	ix
	call	0x00D5
	pop	ix
	ret

	;; GTTRIG -- trigger state of A (0 = the space bar, 1/2 = joystick
	;; buttons). Returns 0 released, 0xFF pressed.
_bios_gttrig::
	push	ix
	call	0x00D8
	pop	ix
	ret

	;; CHSNS -- is a character waiting in the keyboard buffer? The BIOS
	;; answers in the Z flag, which C cannot see, so turn it into 0/1.
_bios_chsns::
	push	ix
	call	0x009C
	pop	ix
	ld	a, #0x00
	ret	Z			; pop/ld leave the flags alone
	inc	a
	ret

	;; CHGET -- fetch one character. Blocks until a key is pressed, so
	;; only call it when bios_chsns() has already said one is waiting.
_bios_chget::
	push	ix
	call	0x009F
	pop	ix
	ret

	;; KILBUF -- throw away anything still sitting in the keyboard buffer,
	;; so a key held down during an animation does not fire a stale action
	;; the moment the game starts reading input again.
_bios_kilbuf::
	push	ix
	call	0x0156
	pop	ix
	ret
