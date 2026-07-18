; Thin cc65-callable thunks over the Commander X16 KERNAL's graphics API
; ($FF20-$FF41). These KERNAL calls use the "16-bit ABI": arguments in the
; virtual registers r0-r4 at zero page $02-$0A. cc65's cx16 target starts
; its own zero page at $22 (see cx16.cfg), so $02-$21 are free for us to
; set directly from C before calling these thunks -- no clobbering.
;
; Geometry (x/y/w/h and line endpoints) is set by the C caller straight
; into r0-r4; these thunks only marshal the A/X/Y/carry inputs and JMP to
; the KERNAL vector (which RTSes back to our caller).

        .export _kgraph_init, _kgraph_clear, _kgraph_setcolors
        .export _kgraph_rect, _kgraph_oval, _kgraph_putchar, _kgraph_line
        .export _kscreen_mode
        .import _g_stroke, _g_fill, _g_bg

r0 = $02

.segment "CODE"

; screen_mode($FF5F): set the screen mode. Mode in .A, carry clear = set.
; Mode $80 (128) is 320x240 @ 256 colours.
_kscreen_mode:
        clc
        jmp $FF5F

; GRAPH_init(0): activate default framebuffer driver, enter graphics mode.
_kgraph_init:
        lda #0
        sta r0
        sta r0+1
        jmp $FF20

; GRAPH_clear: clear the window to the background colour.
_kgraph_clear:
        jmp $FF23

; GRAPH_set_colors(stroke=.A, fill=.X, background=.Y) from the C globals.
_kgraph_setcolors:
        lda _g_stroke
        ldx _g_fill
        ldy _g_bg
        jmp $FF29

; GRAPH_draw_rect: r0=x r1=y r2=w r3=h r4=corner; carry=fill.
; The C fill flag (0/1) arrives in .A; shift its bit0 into carry.
_kgraph_rect:
        lsr a
        jmp $FF2F

; GRAPH_draw_oval: r0=x r1=y r2=w r3=h; carry=fill. Fill flag in .A.
_kgraph_oval:
        lsr a
        jmp $FF35

; GRAPH_put_char: r0=x r1=y, .A=character (advances r0). Char arrives in .A.
_kgraph_putchar:
        jmp $FF41

; GRAPH_draw_line: r0=x1 r1=y1 r2=x2 r3=y2, stroke colour.
_kgraph_line:
        jmp $FF2C
