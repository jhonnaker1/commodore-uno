;----[ main.s - UNO for C64 OS ]--------
; Skeleton: window + menu + draw context + keyboard stubs.
; Verifies the toolchain/boot pipeline before the game engine is added.

         .include "os/h/modules.h"

         #inc_s "app"
         #inc_s "colors"
         #inc_s "ctxdraw"
         #inc_s "io"
         #inc_s "pointer"
         #inc_s "switch"

         #inc_s "screen"
         #inc_s "service"
         #inc_s "input"

;---------------------------------------
;Data Structures

         *= appbase

         .word init     ;App Initializer
         .word msgcmd   ;Message Handler
         .word willquit ;App Clean Up
         .word raw_rts  ;REU Freeze
         .word raw_rts  ;REU Thaw

         .include "src/engine.s"

layer    .word drawmain
         .word sec_rts  ;MouseEvt Handler
         .word kcmdevt  ;Kcmd Evt Handler (control keys)
         .word kprntevt ;KprntEvt handler (printable keys)
         .byte 0        ;Layer Index

drawctx  .word scrbuf      ;Char Origin
         .word colbuf      ;Colr Origin
         .byte screen_cols ;Buff Width
         .byte screen_cols ;Draw Width
         .byte screen_rows ;Draw Height
         .word 0           ;Offset Top
         .word 0           ;Offset Left

title_s   .null "uno"
sub_s     .null "a real c64 os application"
hint_s    .null "press c= n for a new game"
key_label_s .null "last key:"
deal_label_s .null "draw pile:"
hands_label_s .null "hands:"

;PETSCII control codes. Per the Programmer's Guide (tutorial 2, part 2),
;cursor keys/return/delete/home/clr are "printable key events" (KprntEvt),
;NOT Kcmd -- Kcmd is reserved for CONTROL/COMMODORE+key combos and
;function keys. So all game navigation belongs in kprntevt, not kcmdevt.
petscii_crsr_left  = 157
petscii_crsr_right = 29

cursor_min = 2
cursor_max = 37

cursor_x .byte 5   ;test marker column, moved by crsr left/right
last_key .byte 0   ;last PETSCII value seen, shown on screen for verification
entry_a  .byte 0   ;scratch: A as captured at kprntevt entry / from readkprnt

;---------------------------------------

init
         .block
         #ldxy externs
         jsr initextern

         #ldxy layer
         jsr layerpush

         rts
         .bend

willquit
         .block
         rts
         .bend

msgcmd   ;A -> Msg Command
         .block

         #switch 2
         .byte mc_menq,mc_mnu
         .rta mnuenq,mnucmd

done     sec            ;Msg Not Handled
         rts

mnuenq   ;X -> Menu Action Code
         lda #0 ;Enabled, Not Selected
         rts

mnucmd   ;X -> Menu Action Code
         txa
         #switch 2
         .text "n!"
         .rta domenu_new,domenu_quit

         sec ;Action Code Not Recognized
         rts
         .bend

domenu_new
         .block
         jsr newgame
         clc
         rts
         .bend

domenu_quit
         .block
         jsr quitapp
         clc
         rts
         .bend

;---------------------------------------
;Keyboard: control keys (cursor, return,
;delete, function keys, etc.)

kcmdevt
         .block
         jsr readkcmd
         bcs nothing   ;Nothing queued
         jsr deqkcmd   ;Consume it

         ;.A = PETSCII value, .Y = keymods
         sec           ;Not handled yet - stub
         rts
nothing  sec
         rts
         .bend

;---------------------------------------
;Keyboard: printable keys (letters,
;digits, space, punctuation, cursor
;keys, return, delete, home, clr)

kprntevt
         .block
         ;The Programmer's Guide says the OS delivers the PETSCII value
         ;in .A directly when it calls this vector -- but readkprnt_/
         ;deqkprnt_ are the only *documented, verified* way to fetch a
         ;key (confirmed against usingkernal_input). Try the syscall
         ;first; only fall back to whatever arrived in .A on entry if
         ;the queue is already empty (i.e. the OS really did deliver it
         ;via registers and there's nothing left to dequeue).
         sta entry_a

         jsr readkprnt
         bcs got_key    ;Queue empty -- trust entry_a
         sta entry_a    ;Syscall succeeded -- this value is authoritative
         jsr deqkprnt   ;Consume it
got_key
         lda entry_a
         sta last_key

         ;Stir the RNG with every keypress (no jiffy-clock/frame-count
         ;seed available the way the cc65 ports use, since this app
         ;never busy-waits -- the OS calls us per event instead), so a
         ;session's shuffle isn't identical every time.
         jsr rng_next
         lda rng_state
         eor last_key
         sta rng_state

         lda last_key
         cmp #petscii_crsr_left
         bne chk_right
         lda cursor_x
         cmp #cursor_min
         beq redraw
         dec cursor_x
         jmp redraw

chk_right
         cmp #petscii_crsr_right
         bne redraw
         lda cursor_x
         cmp #cursor_max
         beq redraw
         inc cursor_x

redraw
         ldx #0
         jsr markredraw ;Let the OS call drawmain for us

         clc            ;Handled
         rts
         .bend

;---------------------------------------

newgame
         .block
         jsr game_new
         jsr drawmain
         rts
         .bend

drawmain
         .block
         #ldxy drawctx
         jsr setctx

         ldx #d_crsr_h.d_petscr
         ldy #cwhite
         jsr setdprops

         lda #" "
         jsr ctxclear

         #ldxy 3
         clc
         jsr setlrc
         #ldxy 18
         sec
         jsr setlrc

         ldx #d_crsr_h.d_petscr
         ldy #cyellow
         jsr setdprops
         ldx #0
l1       lda title_s,x
         beq l1done
         jsr ctxdraw
         inx
         bne l1
l1done

         #ldxy 6
         clc
         jsr setlrc
         #ldxy 8
         sec
         jsr setlrc

         ldx #d_crsr_h.d_petscr
         ldy #clblue
         jsr setdprops
         ldx #0
l2       lda sub_s,x
         beq l2done
         jsr ctxdraw
         inx
         bne l2
l2done

         #ldxy 10
         clc
         jsr setlrc
         #ldxy 6
         sec
         jsr setlrc

         ldx #d_crsr_h.d_petscr
         ldy #cgreen
         jsr setdprops
         ldx #0
l3       lda hint_s,x
         beq l3done
         jsr ctxdraw
         inx
         bne l3
l3done

         ;Keyboard-input test readout: last PETSCII code seen, and a
         ;marker moved by crsr left/right -- proves kprntevt actually
         ;works before any real game logic depends on it.
         #ldxy 13
         clc
         jsr setlrc
         #ldxy 2
         sec
         jsr setlrc

         ldx #d_crsr_h.d_petscr
         ldy #cblack
         jsr setdprops
         ldx #0
l4       lda key_label_s,x
         beq l4done
         jsr ctxdraw
         inx
         bne l4
l4done
         lda last_key
         jsr print_num3

         #ldxy 15
         clc
         jsr setlrc
         #ldxy cursor_min
         sec
         jsr setlrc

         ldx #d_crsr_h.d_petscr
         ldy #cred
         jsr setdprops
         lda #"*"
         jsr ctxdraw

         #ldxy 15
         clc
         jsr setlrc
         lda cursor_x
         tax
         ldy #0
         sec
         jsr setlrc

         ldx #d_crsr_h.d_petscr
         ldy #cred
         jsr setdprops
         lda #"^"
         jsr ctxdraw

         ;Engine debug readout: proves game_new() actually dealt a real
         ;game (7 cards/hand, plausible draw pile count) before any real
         ;card rendering exists. Removed once the real table/hand UI
         ;replaces it.
         #ldxy 17
         clc
         jsr setlrc
         #ldxy 2
         sec
         jsr setlrc
         ldx #d_crsr_h.d_petscr
         ldy #cblack
         jsr setdprops
         ldx #0
l5       lda deal_label_s,x
         beq l5done
         jsr ctxdraw
         inx
         bne l5
l5done
         lda draw_count
         jsr print_num3

         #ldxy 18
         clc
         jsr setlrc
         #ldxy 2
         sec
         jsr setlrc
         ldx #d_crsr_h.d_petscr
         ldy #cblack
         jsr setdprops
         ldx #0
l6       lda hands_label_s,x
         beq l6done
         jsr ctxdraw
         inx
         bne l6
l6done
         lda hand_count
         jsr print_num3
         lda #" "
         jsr ctxdraw
         lda hand_count+1
         jsr print_num3
         lda #" "
         jsr ctxdraw
         lda hand_count+2
         jsr print_num3
         lda #" "
         jsr ctxdraw
         lda hand_count+3
         jsr print_num3

         rts
         .bend

;A -> value 0-255, drawn as 3 decimal digits (leading zeros) at the
;context's current draw position. Simple debug aid, not part of the
;eventual card UI.
print_num3
         .block
         sta pnval
         lda #0
         sta pnhun
phloop   lda pnval
         cmp #100
         bcc phdone
         sbc #100
         sta pnval
         inc pnhun
         jmp phloop
phdone
         lda #0
         sta pnten
ptloop   lda pnval
         cmp #10
         bcc ptdone
         sbc #10
         sta pnval
         inc pnten
         jmp ptloop
ptdone
         lda pnhun
         clc
         adc #"0"
         jsr ctxdraw
         lda pnten
         clc
         adc #"0"
         jsr ctxdraw
         lda pnval
         clc
         adc #"0"
         jsr ctxdraw
         rts
pnval    .byte 0
pnhun    .byte 0
pnten    .byte 0
         .bend

;---------------------------------------
externs  ;C64 OS KERNAL Link Table

         #inc_h "screen"
layerpush #syscall lscr,layerpush_
markredraw #syscall lscr,markredraw_
setlrc    #syscall lscr,setlrc_
setdprops #syscall lscr,setdprops_
ctxclear  #syscall lscr,ctxclear_
ctxdraw   #syscall lscr,ctxdraw_

         #inc_h "service"
quitapp  #syscall lser,quitapp_

         #inc_h "toolkit"
setctx   #syscall ltkt,setctx_

         #inc_h "input"
readkcmd  #syscall linp,readkcmd_
deqkcmd   #syscall linp,deqkcmd_
readkprnt #syscall linp,readkprnt_
deqkprnt  #syscall linp,deqkprnt_

         .byte $ff ;terminator
