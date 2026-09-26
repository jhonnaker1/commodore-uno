; app.s - the C64 OS side of UNO: application header, layer, KERNAL link
; table, and the trampolines between the OS and the C in main.c.
;
; Every constant here is from the SDK in ../c64os/inc/os -- app.s,
; service.s, screen.h, input.h -- not inferred from a binary.
;
; ZERO PAGE. llvm-mos keeps 32 imaginary registers in zero page, and C64
; OS's own map (//os/docs/memory.t, on the C64 OS disk) leaves no free run
; that long. So the registers sit at $4E-$6D -- BASIC's floating-point
; accumulators, which C64 OS touches only inside its float library -- and
; every crossing swaps them: an OS->C entry saves the OS's 32 bytes, a C->OS
; call puts them back, and the C side keeps its own copy per nesting depth.
;
; Swapping protects every call, but not an INTERRUPT, which runs in the
; middle of C with C's values in place. That is what ruled out $02-$21,
; the first choice: C64 OS's timers decrement $02 and $09 from the IRQ,
; which moved C's soft stack pointer underneath it and scattered stray
; bytes through the program. No code reachable from C64 OS's IRQ handler
; ($0314 -> $CE8B) touches $4E-$6D; that was checked by crawling it.

initextern = 0x02FC
raw_rts    = 0x02B2
sec_rts    = 0x02B3

ZP_BASE    = 0x4E             ; must match __rc0 in c64os.ld
ZP_LEN     = 32
MAX_DEPTH  = 4                ; nested OS->C entries we can hold state for

; ---------------------------------------------------------------------------
; The five application vectors. c64os.ld puts this section first, at $0900.

    .section .app_header,"a",@progbits
    .short  app_init          ; App Initializer
    .short  app_msgcmd        ; Message Handler
    .short  app_willquit      ; App Clean Up
    .short  raw_rts           ; REU Freeze
    .short  raw_rts           ; REU Thaw

    .text

app_init:
    ldx     #mos16lo(externs)
    ldy     #mos16hi(externs)
    jsr     initextern

    ; Stack sentinel: the section loads as zeros; $A5 marks every byte the
    ; soft stack has never reached. Absolute,Y through a patched operand,
    ; so not even this touches the OS's zero page.
    lda     #mos16hi(__stack_bottom)
    sta     fill+2
    ldx     #2                ; 512 bytes = two pages
    ldy     #0
    lda     #0xA5
fill:
    sta     __stack_bottom,y
    iny
    bne     fill
    inc     fill+2
    dex
    bne     fill

    ldx     #mos16lo(layer)
    ldy     #mos16hi(layer)
    jsr     os_layerpush

    lda     #mos16lo(uno_start)
    ldx     #mos16hi(uno_start)
    jmp     enter

app_willquit:
    rts

; A -> message command, X -> menu action code. C returns A in its low
; byte and the carry to hand back in bit 0 of its high byte.
app_msgcmd:
    sta     arg_a
    stx     arg_x
    lda     #mos16lo(uno_msgcmd)
    ldx     #mos16hi(uno_msgcmd)
    jsr     enter
    pha
    txa
    lsr     a                 ; C <- bit 0 of the high byte
    pla
    rts

; C64 OS calls the draw callback with $01 = $34 -- I/O banked out -- so
; $D800 is its colour BUFFER in RAM, which is what main.c writes.
layer_draw:
    lda     #mos16lo(uno_draw)
    ldx     #mos16hi(uno_draw)
    jmp     enter

; A -> the key C64 OS delivered with the event (main.c prefers the queue).
layer_kprnt:
    sta     arg_a
    lda     #mos16lo(uno_key)
    ldx     #mos16hi(uno_key)
    jsr     enter
    clc                       ; handled
    rts

; ---------------------------------------------------------------------------
; OS -> C. A/X = the C function; its arguments are in arg_a/arg_x. Returns
; the C function's A/X.

enter:
    sta     target
    stx     target+1
    cld

    ldx     #ZP_LEN-1         ; the OS's zero page, kept for the way out
1:  lda     ZP_BASE,x
    sta     os_zp,x
    dex
    bpl     1b

    lda     depth
    bne     2f
    ; Outermost: a fresh soft stack. The other registers are scratch to C.
    lda     #mos16lo(__stack)
    sta     ZP_BASE
    lda     #mos16hi(__stack)
    sta     ZP_BASE+1
    jmp     3f
2:  ; Nested, from inside one of our own OS calls: carry on from the
    ; registers the outer C had when it made that call, so this frame
    ; goes below its stack rather than over it.
    jsr     slot_offset
    ldx     #0
4:  lda     app_zp,y
    sta     ZP_BASE,x
    iny
    inx
    cpx     #ZP_LEN
    bne     4b
3:
    inc     depth
    lda     arg_a
    ldx     arg_x
    jsr     call_target
    sta     arg_a
    stx     arg_x
    dec     depth

    ldx     #ZP_LEN-1
1:  lda     os_zp,x
    sta     ZP_BASE,x
    dex
    bpl     1b

    lda     arg_a
    ldx     arg_x
    rts

call_target:
    jmp     (target)

; Y <- (depth-1) * 32, the C side's save slot for the current depth.
slot_offset:
    lda     depth
    sec
    sbc     #1
    asl     a
    asl     a
    asl     a
    asl     a
    asl     a
    tay
    rts

; C -> OS, first half: park C's zero page in its slot, restore the OS's.
; Preserves nothing.
to_os:
    jsr     slot_offset
    ldx     #0
1:  lda     ZP_BASE,x
    sta     app_zp,y
    iny
    inx
    cpx     #ZP_LEN
    bne     1b
    ldx     #ZP_LEN-1
2:  lda     os_zp,x
    sta     ZP_BASE,x
    dex
    bpl     2b
    rts

; C -> OS, second half: keep whatever the OS left, bring C's back.
from_os:
    ldx     #ZP_LEN-1
1:  lda     ZP_BASE,x
    sta     os_zp,x
    dex
    bpl     1b
    jsr     slot_offset
    ldx     #0
2:  lda     app_zp,y
    sta     ZP_BASE,x
    iny
    inx
    cpx     #ZP_LEN
    bne     2b
    rts

; ---------------------------------------------------------------------------
; The OS calls main.c makes. Arguments and results travel through the
; same bytes the trampolines use, so no register survives a swap by luck.

; markredraw with the exact registers the assembly port has always passed
; (X = 0, A = 1) -- verified playable there.
    .globl  os_markredraw
os_markredraw:
    jsr     to_os
    ldx     #0
    lda     #1
    jsr     os_markredraw_
    jmp     from_os

    .globl  os_quitapp
os_quitapp:
    jsr     to_os
    jsr     os_quitapp_
    jmp     from_os

; Returns the key in A, and X = 1 when the queue was empty (carry set).
    .globl  os_readkprnt
os_readkprnt:
    jsr     to_os
    jsr     os_readkprnt_
    sta     res_a
    lda     #0
    rol     a
    sta     res_x
    jsr     from_os
    lda     res_a
    ldx     res_x
    rts

    .globl  os_deqkprnt
os_deqkprnt:
    jsr     to_os
    jsr     os_deqkprnt_
    jmp     from_os

; ---------------------------------------------------------------------------
; Buffers -> screen, now. C64 OS draws layers into buffers -- characters at
; $0400, colours in the RAM under I/O at $D800 -- and copies them to the
; screen the VIC actually shows (the RAM under I/O at $DC00: $DD00 = $C4,
; $D018 = $75) at the end of each event-loop pass. A CPU turn runs inside
; one event, so main.c calls this to show each move as it happens. The
; OS's own redraw_ would call back into C from inside C, which llvm-mos's
; static stack frames do not allow for.
;
; Stops at $DFE7: $DFF8 up are the sprite pointers, and the mouse pointer
; is a sprite.

    .globl  os_present
os_present:
    php
    sei
    lda     0x01
    pha
    and     #0xF8
    ora     #0x04             ; all RAM: $DC00 and $D800 are the RAM under I/O
    sta     0x01
    ldx     #0
1:  lda     0x0400,x
    sta     0xDC00,x
    lda     0x0500,x
    sta     0xDD00,x
    lda     0x0600,x
    sta     0xDE00,x
    inx
    bne     1b
2:  lda     0x0700,x
    sta     0xDF00,x
    inx
    cpx     #0xE8
    bne     2b

    ; Colours: RAM $D800 -> colour RAM, the I/O at the same address. One
    ; page at a time through a bounce buffer, flipping the bank between.
    lda     #0xD8
    sta     5f+2              ; source page, read with all RAM
    sta     6f+2              ; target page, written with I/O in
    ldy     #4
3:  ldx     #0
5:  lda     0xD800,x
    sta     bounce,x
    inx
    bne     5b
    lda     0x01
    ora     #0x01             ; %101: I/O in
    sta     0x01
4:  lda     bounce,x
6:  sta     0xD800,x
    inx
    bne     4b
    lda     0x01
    and     #0xF8
    ora     #0x04
    sta     0x01
    inc     5b+2
    inc     6b+2
    dey
    bne     3b

    pla
    sta     0x01
    plp
    rts

; ---------------------------------------------------------------------------

    .data
layer:
    .short  layer_draw        ; draw
    .short  sec_rts           ; mouse event: not handled
    .short  sec_rts           ; Kcmd event (Control/C= combos): not handled
    .short  layer_kprnt       ; Kprnt event (printable + cursor keys)
    .byte   0                 ; layer index

; The KERNAL link table. C64 OS rewrites each record into a JMP, so these
; labels are both descriptors and call targets.
externs:
os_layerpush:   .byte 0xF6    ; lscr
                .short 0x0006 ; layerpush_
os_markredraw_: .byte 0xF6    ; lscr
                .short 0x0003 ; markredraw_
os_quitapp_:    .byte 0xF2    ; lser
                .short 0x0021 ; quitapp_
os_readkprnt_:  .byte 0xFC    ; linp
                .short 0x0018 ; readkprnt_
os_deqkprnt_:   .byte 0xFC    ; linp
                .short 0x001B ; deqkprnt_
                .byte 0xFF

depth:  .byte 0
target: .short 0
arg_a:  .byte 0
arg_x:  .byte 0
res_a:  .byte 0
res_x:  .byte 0
os_zp:  .fill ZP_LEN, 1, 0
app_zp: .fill ZP_LEN * MAX_DEPTH, 1, 0
bounce: .fill 256, 1, 0       ; os_present's colour copy
