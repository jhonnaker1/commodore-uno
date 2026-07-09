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
;digits, space, punctuation)

kprntevt
         .block
         jsr readkprnt
         bcs nothing   ;Nothing queued
         jsr deqkprnt  ;Consume it

         ;.A = PETSCII value
         sec           ;Not handled yet - stub
         rts
nothing  sec
         rts
         .bend

;---------------------------------------

newgame
         .block
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

         rts
         .bend

;---------------------------------------
externs  ;C64 OS KERNAL Link Table

         #inc_h "screen"
layerpush #syscall lscr,layerpush_
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
