; wait_vsync for the Commander X16.
;
; cc65's cx16 target has no waitvsync(), so this polls VERA's VSYNC
; interrupt-status flag (ISR bit0 at $9F27) directly. It masks IRQs first
; (SEI) so the KERNAL's own vsync IRQ handler can't clear the flag out from
; under the poll -- the flag is still SET by VERA hardware while IRQs are
; masked, so this reliably catches exactly one vsync, then restores the
; previous interrupt state. Masking for at most one frame (~16ms) is
; harmless; the delayed KERNAL keyboard scan is imperceptible.

.export _wait_vsync

VERA_ISR = $9F27

.proc _wait_vsync
        php
        sei
        lda #$01
        sta VERA_ISR        ; clear any pending vsync flag
@wait:  lda VERA_ISR
        and #$01
        beq @wait           ; spin until VERA sets vsync
        plp                 ; restore caller's interrupt-enable state
        rts
.endproc
