;----[ main.s - UNO for C64 OS ]--------
; Real windowed C64 OS application: menu bar, draw-context rendering,
; event-driven keyboard input, and the full UNO turn loop (src/engine.s
; holds the platform-independent game rules).

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
hint_s    .null "press space for a new game"

draw_label_s  .null "draw:"
top_label_s   .null "top:"
col_label_s   .null "col:"
dir_fwd_s     .null "dir ->"
dir_rev_s     .null "dir <-"
hand_label_s  .null "your hand:"
turn_msg_s    .null "your turn"
invalid_msg_s .null "that card does not match!"
colorpick_s   .null "choose a color:"
challenge1_s  .null "wild draw four played."
challenge2_s  .null "challenge?"
yes_s         .null "yes"
no_s          .null "no"
win_s         .null "you win!"
lose_s        .null "cpu wins."
playagain_s   .null "press space to play again"
colorname_red_s    .null "red"
colorname_yellow_s .null "yellow"
colorname_green_s  .null "green"
colorname_blue_s   .null "blue"

;PETSCII control codes. Per the Programmer's Guide (tutorial 2, part 2),
;cursor keys/return/delete/home/clr are "printable key events" (KprntEvt),
;NOT Kcmd -- Kcmd is reserved for CONTROL/COMMODORE+key combos and
;function keys. So all game navigation belongs in kprntevt, not kcmdevt.
petscii_crsr_left  = 157
petscii_crsr_right = 29
petscii_crsr_up    = 145

;UI state machine, driven from kprntevt and rendered by drawmain.
;0=title screen, 1=hand selection (human's turn), 2=wild color picker,
;3=wild draw four challenge prompt, 4=game over screen.
ui_state       .byte 0
cursor_hand    .byte 0
pending_hand_idx .byte 0
color_sel      .byte 0
challenge_sel  .byte 0
invalid_flash  .byte 0

last_key .byte 0   ;last PETSCII value seen
entry_a  .byte 0   ;scratch: A as captured at kprntevt entry / from readkprnt

;.block/.bend scopes ALL labels inside it, including plain data bytes,
;not just code -- so state shared between draw_hand/dh_pos/
;draw_one_hand_card (three separate blocks) has to live at file scope.
dh_i        .byte 0
dh_row      .byte 0
dh_col      .byte 0
dh_card     .byte 0
dh_selected .byte 0

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
         jsr start_new_game
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
;Keyboard: control keys (CONTROL/COMMODORE
;+key combos, function keys). Unused so far
;-- all game input is printable-key events.

kcmdevt
         .block
         sec
         rts
         .bend

;---------------------------------------
;Keyboard: printable keys (letters, digits,
;space, punctuation, cursor keys, return,
;delete, home, clr)

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

         lda ui_state
         cmp #0
         beq ke_title
         cmp #1
         beq ke_handselect
         cmp #2
         beq ke_colorpicker
         cmp #3
         beq ke_challenge

         ;state 4 (game over) and anything else: space restarts
         lda last_key
         cmp #" "
         bne ke_done
         jsr start_new_game
ke_done  clc
         rts

ke_title
         lda last_key
         cmp #" "
         bne ke_done
         jsr start_new_game
         clc
         rts

ke_handselect
         jsr handle_hand_key
         clc
         rts

ke_colorpicker
         jsr handle_color_key
         clc
         rts

ke_challenge
         jsr handle_challenge_key
         clc
         rts
         .bend

;---------------------------------------
;Human-turn input handling (ui_state==1)

handle_hand_key
         .block
         lda #0
         sta invalid_flash

         lda last_key
         cmp #petscii_crsr_left
         bne hhk_right
         lda cursor_hand
         beq hhk_redraw
         dec cursor_hand
         jmp hhk_redraw

hhk_right
         cmp #petscii_crsr_right
         bne hhk_up
         lda cursor_hand
         clc
         adc #1
         cmp hand_count
         bcs hhk_redraw
         inc cursor_hand
         jmp hhk_redraw

hhk_up
         cmp #petscii_crsr_up
         bne hhk_fire
         jsr do_draw_action
         rts

hhk_fire
         cmp #" "
         beq hhk_doplay
         cmp #13
         beq hhk_doplay

         jsr quick_select_index
         cmp #g_none
         beq hhk_redraw
         cmp hand_count
         bcs hhk_redraw
         sta cursor_hand
         jsr try_play_selected
         rts

hhk_doplay
         jsr try_play_selected
         rts

hhk_redraw
         ldx #0
         jsr markredraw
         rts
         .bend

;last_key -> A = 0-19 hand slot, or g_none if not a quick-select key.
quick_select_index
         .block
         lda last_key
         cmp #"0"
         bne qsi_chkdigit
         lda #9
         rts
qsi_chkdigit
         lda last_key
         cmp #"1"
         bcc qsi_chkupper
         cmp #"9"+1
         bcs qsi_chkupper
         sec
         sbc #"1"
         rts
qsi_chkupper
         lda last_key
         cmp #"A"
         bcc qsi_chklower
         cmp #"J"+1
         bcs qsi_chklower
         sec
         sbc #"A"
         clc
         adc #10
         rts
qsi_chklower
         lda last_key
         cmp #"a"
         bcc qsi_none
         cmp #"j"+1
         bcs qsi_none
         sec
         sbc #"a"
         clc
         adc #10
         rts
qsi_none
         lda #g_none
         rts
         .bend

;Draws a card for the human (player 0). Auto-plays it if legal
;(prompting for a wild color first if needed); otherwise ends the turn.
do_draw_action
         .block
         ldx #0
         jsr draw_card
         sta dda_card

         jsr is_legal
         cmp #1
         beq dda_autoplay

         jsr end_turn_no_play
         jsr after_action
         rts

dda_autoplay
         lda hand_count
         sec
         sbc #1
         sta pending_hand_idx

         lda dda_card
         lsr a
         lsr a
         lsr a
         lsr a
         cmp #color_wild
         bne dda_notwild

         lda #2
         sta ui_state
         lda #0
         sta color_sel
         ldx #0
         jsr markredraw
         rts

dda_notwild
         ldx pending_hand_idx
         lda #0
         jsr play_card
         jsr after_action
         rts
dda_card .byte 0
         .bend

;Attempts to play the currently-selected hand card. Shows an invalid
;message and stays put if it's not a legal play.
try_play_selected
         .block
         lda cursor_hand
         tay
         lda player_hands,y
         sta tps_card

         jsr is_legal
         cmp #1
         beq tps_legal

         lda #1
         sta invalid_flash
         ldx #0
         jsr markredraw
         rts

tps_legal
         lda #0
         sta invalid_flash
         lda cursor_hand
         sta pending_hand_idx

         lda tps_card
         lsr a
         lsr a
         lsr a
         lsr a
         cmp #color_wild
         bne tps_notwild

         lda #2
         sta ui_state
         lda #0
         sta color_sel
         ldx #0
         jsr markredraw
         rts

tps_notwild
         ldx pending_hand_idx
         lda #0
         jsr play_card
         jsr after_action
         rts
tps_card .byte 0
         .bend

;---------------------------------------
;Wild color picker (ui_state==2)

handle_color_key
         .block
         lda last_key
         cmp #petscii_crsr_left
         bne hck_right
         lda color_sel
         beq hck_wrapleft
         dec color_sel
         jmp hck_redraw
hck_wrapleft
         lda #3
         sta color_sel
         jmp hck_redraw

hck_right
         cmp #petscii_crsr_right
         bne hck_fire
         lda color_sel
         cmp #3
         beq hck_wrapright
         inc color_sel
         jmp hck_redraw
hck_wrapright
         lda #0
         sta color_sel
         jmp hck_redraw

hck_fire
         cmp #" "
         beq hck_confirm
         cmp #13
         beq hck_confirm
         rts

hck_confirm
         ldx pending_hand_idx
         lda color_sel
         jsr play_card
         jsr after_action
         rts

hck_redraw
         ldx #0
         jsr markredraw
         rts
         .bend

;---------------------------------------
;Wild Draw Four challenge prompt (ui_state==3)

handle_challenge_key
         .block
         lda last_key
         cmp #petscii_crsr_left
         beq hxk_toggle
         cmp #petscii_crsr_right
         beq hxk_toggle
         cmp #" "
         beq hxk_confirm
         cmp #13
         beq hxk_confirm
         rts

hxk_toggle
         lda challenge_sel
         eor #1
         sta challenge_sel
         ldx #0
         jsr markredraw
         rts

hxk_confirm
         lda challenge_sel
         jsr resolve_wd4
         jsr after_action
         rts
         .bend

;---------------------------------------
;Central post-action hub: checks for a win, a pending Wild Draw Four
;decision (AI-resolves it or prompts the human), then runs CPU turns
;(no per-turn delay -- see README's timer note) until it's the human's
;turn again or the game ends.

after_action
         .block
aa_loop
         lda flag_win
         beq aa_chkwd4
         lda #4
         sta ui_state
         jmp aa_redraw

aa_chkwd4
         lda wd4_pending
         beq aa_chkturn
         lda wd4_victim
         bne aa_ai_wd4
         lda #3
         sta ui_state
         lda #0
         sta challenge_sel
         jmp aa_redraw
aa_ai_wd4
         ldx wd4_victim
         jsr ai_should_challenge_wd4
         jsr resolve_wd4
         lda #1
         sta ui_state
         jsr drawmain
         jsr busy_wait_short
         jmp aa_loop

aa_chkturn
         lda current_player
         beq aa_human
         jsr cpu_take_turn
         lda #1
         sta ui_state
         jsr drawmain
         jsr busy_wait_short
         jmp aa_loop

aa_human
         lda #1
         sta ui_state
         lda #0
         sta invalid_flash
         lda cursor_hand
         cmp hand_count
         bcc aa_redraw
         lda #0
         sta cursor_hand

aa_redraw
         ldx #0
         jsr markredraw
         rts
         .bend

;Takes one full CPU turn for current_player (must be 1-3).
cpu_take_turn
         .block
         lda current_player
         sta ctt_player

         ldx ctt_player
         jsr ai_choose_card
         cmp #g_none
         bne ctt_hasidx

         ldx ctt_player
         jsr draw_card
         sta ctt_card
         jsr is_legal
         cmp #1
         beq ctt_drawplay
         jsr end_turn_no_play
         rts

ctt_drawplay
         ldx ctt_player
         lda hand_count,x
         sec
         sbc #1
         sta ctt_idx
         lda ctt_card
         lsr a
         lsr a
         lsr a
         lsr a
         cmp #color_wild
         bne ctt_draw_notwild
         ldx ctt_player
         jsr ai_choose_color
         jmp ctt_draw_havecolor
ctt_draw_notwild
         lda #0
ctt_draw_havecolor
         pha
         ldx ctt_idx
         pla
         jsr play_card
         rts

ctt_hasidx
         sta ctt_idx
         ldx ctt_player
         jsr mul40
         clc
         adc ctt_idx
         tay
         lda player_hands,y
         sta ctt_card
         lsr a
         lsr a
         lsr a
         lsr a
         cmp #color_wild
         bne ctt_notwild
         ldx ctt_player
         jsr ai_choose_color
         jmp ctt_havecolor
ctt_notwild
         lda #0
ctt_havecolor
         pha
         ldx ctt_idx
         pla
         jsr play_card
         rts

ctt_player .byte 0
ctt_card   .byte 0
ctt_idx    .byte 0
         .bend

start_new_game
         .block
         jsr game_new
         lda #0
         sta cursor_hand
         sta invalid_flash
         lda #1
         sta ui_state
         jsr drawmain
         rts
         .bend

;A short, blocking delay (roughly two-thirds of a second at ~1MHz --
;3 passes of a ~256*256-iteration nested loop) so CPU turns and
;automatic Wild Draw Four resolutions are visible one at a time --
;markredraw_ only *schedules* a redraw for whenever the OS's event
;loop next gets to it, which never happens while this code is still
;running in a tight loop, so the caller forces an immediate jsr
;drawmain and pauses here before continuing. Deliberately NOT
;timer-based (see README's note on why); keep it brief enough that a
;chain of several CPU turns doesn't add up to the couple of seconds
;C64 OS treats as an unresponsive app.
busy_wait_short
         .block
         lda #3
         sta bws_reps
bws_replus
         jsr bws_one_pass
         dec bws_reps
         bne bws_replus
         rts

bws_one_pass
         ldx #0
bws_outer
         ldy #0
bws_inner
         dey
         bne bws_inner
         dex
         bne bws_outer
         rts

bws_reps .byte 0
         .bend

;---------------------------------------
;Small rendering helpers

;A=row, X=col (both plain bytes, 0-based; high byte assumed 0).
;Positions the draw cursor for a subsequent ctxdraw/draw_str run.
goto_rc
         .block
         stx grc_col
         tax
         ldy #0
         clc
         jsr setlrc
         ldx grc_col
         ldy #0
         sec
         jsr setlrc
         rts
grc_col .byte 0
         .bend

;A=low byte of a null-terminated string's address, X=high byte. Draws
;it at the current position via self-modified absolute,Y addressing
;(same technique the vendored SDK's own drawbox.s macro uses) since a
;runtime pointer needs a zero-page location for true (zp),y indirection
;and none is safely known to be free here.
draw_str
         .block
         sta ds_lda+1
         stx ds_lda+2
         ldy #0
ds_loop
ds_lda   lda $ffff,y
         beq ds_done
         jsr ctxdraw
         iny
         bne ds_loop
ds_done
         rts
         .bend

;A -> card byte. Returns the display char: '0'-'9', or S/V/D/W/F for
;Skip/Reverse/DrawTwo/Wild/WildDrawFour.
value_char
         .block
         and #$0f
         cmp #10
         bcc vc_digit
         cmp #val_skip
         bne vc_chkrev
         lda #"S"
         rts
vc_chkrev
         cmp #val_reverse
         bne vc_chkdraw2
         lda #"V"
         rts
vc_chkdraw2
         cmp #val_draw2
         bne vc_chkwild
         lda #"D"
         rts
vc_chkwild
         cmp #val_wild
         bne vc_wild4
         lda #"W"
         rts
vc_wild4
         lda #"F"
         rts
vc_digit
         clc
         adc #"0"
         rts
         .bend

;A -> suit color (0-3). Returns R/Y/G/B.
suit_letter
         .block
         cmp #color_red
         bne sl_chky
         lda #"R"
         rts
sl_chky
         cmp #color_yellow
         bne sl_chkg
         lda #"Y"
         rts
sl_chkg
         cmp #color_green
         bne sl_chkb
         lda #"G"
         rts
sl_chkb
         lda #"B"
         rts
         .bend

;A -> suit color (0-3, or 4=wild). Returns a C64 OS ink color constant.
;Wild returns cblack; callers dealing with a played wild card should
;pass the resolved top_color instead of calling this on color_wild.
color_ink
         .block
         cmp #color_red
         bne ci_chky
         lda #cred
         rts
ci_chky
         cmp #color_yellow
         bne ci_chkg
         lda #cyellow
         rts
ci_chkg
         cmp #color_green
         bne ci_chkb
         lda #cgreen
         rts
ci_chkb
         cmp #color_blue
         bne ci_wild
         lda #cblue
         rts
ci_wild
         lda #cblack
         rts
         .bend

;A -> hand slot index (0-19). Returns the quick-play key char for it:
;'1'-'9', '0', then 'A'-'J'.
label_char
         .block
         cmp #10
         bcs lc_ge10
         cmp #9
         beq lc_zero
         clc
         adc #"1"
         rts
lc_zero
         lda #"0"
         rts
lc_ge10
         sec
         sbc #10
         clc
         adc #"A"
         rts
         .bend

;---------------------------------------
;Screen drawing

drawmain
         .block
         #ldxy drawctx
         jsr setctx

         lda ui_state
         bne dm_ingame
         jsr draw_title_screen
         rts

dm_ingame
         lda ui_state
         cmp #4
         beq dm_gameover
         jsr draw_game_screen
         rts

dm_gameover
         jsr draw_game_over_screen
         rts
         .bend

draw_title_screen
         .block
         ldx #d_crsr_h.d_petscr
         ldy #cblack
         jsr setdprops
         lda #" "
         jsr ctxclear

         lda #3
         ldx #18
         jsr goto_rc
         ldx #d_crsr_h.d_petscr
         ldy #cyellow
         jsr setdprops
         lda #<title_s
         ldx #>title_s
         jsr draw_str

         lda #6
         ldx #8
         jsr goto_rc
         ldx #d_crsr_h.d_petscr
         ldy #clblue
         jsr setdprops
         lda #<sub_s
         ldx #>sub_s
         jsr draw_str

         lda #10
         ldx #6
         jsr goto_rc
         ldx #d_crsr_h.d_petscr
         ldy #cgreen
         jsr setdprops
         lda #<hint_s
         ldx #>hint_s
         jsr draw_str

         rts
         .bend

draw_game_screen
         .block
         ldx #d_crsr_h.d_petscr
         ldy #cblack
         jsr setdprops
         lda #" "
         jsr ctxclear

         jsr draw_opponents
         jsr draw_table

         lda ui_state
         cmp #2
         beq dgs_colorpicker
         cmp #3
         beq dgs_challenge
         jsr draw_status_msg
         jmp dgs_hand
dgs_colorpicker
         jsr draw_color_picker
         jmp dgs_hand
dgs_challenge
         jsr draw_challenge_prompt
dgs_hand
         jsr draw_hand
         rts
         .bend

;Row 5: "YOUR TURN" or the invalid-card warning.
draw_status_msg
         .block
         lda #5
         ldx #1
         jsr goto_rc
         ldx #d_crsr_h.d_petscr
         ldy #cblack
         jsr setdprops

         lda invalid_flash
         beq dsm_normal
         ldy #cred
         jsr setdprops
         lda #<invalid_msg_s
         ldx #>invalid_msg_s
         jsr draw_str
         rts
dsm_normal
         lda #<turn_msg_s
         ldx #>turn_msg_s
         jsr draw_str
         rts
         .bend

draw_opponents
         .block
         ldx #d_crsr_h.d_petscr
         ldy #cblack
         jsr setdprops

         lda #0
         sta do_i
do_loop
         lda do_i
         cmp #3
         beq do_done

         tay
         lda #1
         ldx opp_x_table,y
         jsr goto_rc

         lda #"C"
         jsr ctxdraw
         lda #"P"
         jsr ctxdraw
         lda #"U"
         jsr ctxdraw
         lda do_i
         clc
         adc #"1"
         jsr ctxdraw
         lda #":"
         jsr ctxdraw

         lda do_i
         clc
         adc #1
         tax
         lda hand_count,x
         jsr print_num2

         lda #" "
         jsr ctxdraw
         lda do_i
         clc
         adc #1
         cmp current_player
         bne do_noturn
         lda #"<"
         jmp do_turnchar
do_noturn
         lda #" "
do_turnchar
         jsr ctxdraw

         inc do_i
         jmp do_loop
do_done
         rts
do_i .byte 0
opp_x_table .byte 1,14,27
         .bend

draw_table
         .block
         lda #3
         ldx #1
         jsr goto_rc
         ldx #d_crsr_h.d_petscr
         ldy #cblack
         jsr setdprops
         lda #<draw_label_s
         ldx #>draw_label_s
         jsr draw_str
         lda draw_count
         jsr print_num3

         lda #3
         ldx #15
         jsr goto_rc
         ldx #d_crsr_h.d_petscr
         ldy #cblack
         jsr setdprops
         lda #<top_label_s
         ldx #>top_label_s
         jsr draw_str
         lda #"["
         jsr ctxdraw

         lda top_card
         lsr a
         lsr a
         lsr a
         lsr a
         cmp #color_wild
         bne dt_notwild
         lda top_color
         jmp dt_gotcolor
dt_notwild
         lda top_card
         lsr a
         lsr a
         lsr a
         lsr a
dt_gotcolor
         jsr color_ink
         tay
         ldx #d_crsr_h.d_petscr
         jsr setdprops

         lda top_color
         jsr suit_letter
         jsr ctxdraw
         lda top_card
         jsr value_char
         jsr ctxdraw

         ldx #d_crsr_h.d_petscr
         ldy #cblack
         jsr setdprops
         lda #"]"
         jsr ctxdraw

         lda #3
         ldx #25
         jsr goto_rc
         ldx #d_crsr_h.d_petscr
         ldy #cblack
         jsr setdprops
         lda #<col_label_s
         ldx #>col_label_s
         jsr draw_str
         lda top_color
         jsr color_ink
         tay
         ldx #d_crsr_h.d_petscr
         jsr setdprops
         lda top_color
         jsr suit_letter
         jsr ctxdraw

         ldx #d_crsr_h.d_petscr
         ldy #cblack
         jsr setdprops
         lda #3
         ldx #33
         jsr goto_rc
         lda direction
         cmp #1
         bne dt_revdir
         lda #<dir_fwd_s
         ldx #>dir_fwd_s
         jmp dt_dirdraw
dt_revdir
         lda #<dir_rev_s
         ldx #>dir_rev_s
dt_dirdraw
         jsr draw_str
         rts
         .bend

;Row 8: "YOUR HAND:07", then up to 20 cards, 6 per row, from row 9.
draw_hand
         .block
         lda #8
         ldx #1
         jsr goto_rc
         ldx #d_crsr_h.d_petscr
         ldy #cblack
         jsr setdprops
         lda #<hand_label_s
         ldx #>hand_label_s
         jsr draw_str
         lda hand_count
         jsr print_num2

         lda #0
         sta dh_i
dh_loop
         lda dh_i
         cmp hand_count
         bcs dh_done
         cmp #20
         bcs dh_done

         jsr dh_pos
         lda dh_row
         ldx dh_col
         jsr goto_rc

         lda dh_i
         tay
         lda player_hands,y
         sta dh_card

         lda dh_i
         cmp cursor_hand
         bne dh_notsel
         lda #1
         jmp dh_selflag
dh_notsel
         lda #0
dh_selflag
         sta dh_selected

         jsr draw_one_hand_card

         inc dh_i
         jmp dh_loop
dh_done
         rts
         .bend

;Computes dh_row/dh_col from dh_i (0-19), 6 cards per row from row 9.
dh_pos
         .block
         lda dh_i
         sta dhp_rem
         lda #0
         sta dhp_div
dhp_loop
         lda dhp_rem
         cmp #6
         bcc dhp_done
         sbc #6
         sta dhp_rem
         inc dhp_div
         jmp dhp_loop
dhp_done
         lda dhp_div
         clc
         adc #9
         sta dh_row

         lda dhp_rem
         asl a
         asl a
         sta dhp_tmp
         lda dhp_rem
         asl a
         clc
         adc dhp_tmp
         clc
         adc #1
         sta dh_col
         rts
dhp_rem .byte 0
dhp_div .byte 0
dhp_tmp .byte 0
         .bend

;Draws one hand card at the already-positioned cursor. Uses dh_card
;and dh_selected (set by draw_hand).
draw_one_hand_card
         .block
         lda dh_selected
         beq doh_noinv

         lda dh_card
         lsr a
         lsr a
         lsr a
         lsr a
         cmp #color_wild
         bne doh_inv_notwild
         lda #cblack
         jmp doh_inv_gotcolor
doh_inv_notwild
         lda dh_card
         lsr a
         lsr a
         lsr a
         lsr a
         jsr color_ink
doh_inv_gotcolor
         tay
         ldx #d_crsr_h.d_petscr.d_revers
         jsr setdprops
         jmp doh_draw

doh_noinv
         lda dh_card
         lsr a
         lsr a
         lsr a
         lsr a
         cmp #color_wild
         bne doh_notwild
         lda #cblack
         jmp doh_gotcolor
doh_notwild
         lda dh_card
         lsr a
         lsr a
         lsr a
         lsr a
         jsr color_ink
doh_gotcolor
         tay
         ldx #d_crsr_h.d_petscr
         jsr setdprops

doh_draw
         lda #"["
         jsr ctxdraw
         lda dh_i
         jsr label_char
         jsr ctxdraw
         lda #":"
         jsr ctxdraw

         lda dh_card
         lsr a
         lsr a
         lsr a
         lsr a
         cmp #color_wild
         beq doh_wildletter
         jsr suit_letter
         jmp doh_letterdone
doh_wildletter
         lda #"?"
doh_letterdone
         jsr ctxdraw

         lda dh_card
         jsr value_char
         jsr ctxdraw
         lda #"]"
         jsr ctxdraw
         rts
         .bend

;Row 5-6: wild color picker, cursor left/right cycles, space/enter picks.
draw_color_picker
         .block
         lda #5
         ldx #1
         jsr goto_rc
         ldx #d_crsr_h.d_petscr
         ldy #cblack
         jsr setdprops
         lda #<colorpick_s
         ldx #>colorpick_s
         jsr draw_str

         lda #6
         ldx #1
         jsr goto_rc

         lda #0
         sta dcp_i
dcp_loop
         lda dcp_i
         cmp #4
         beq dcp_done

         ldx #d_crsr_h.d_petscr
         ldy #cblack
         jsr setdprops
         lda dcp_i
         cmp color_sel
         bne dcp_nosel
         lda #"["
         jmp dcp_bracket1
dcp_nosel
         lda #" "
dcp_bracket1
         jsr ctxdraw

         lda dcp_i
         jsr color_ink
         tay
         ldx #d_crsr_h.d_petscr
         jsr setdprops

         lda dcp_i
         jsr dcp_name
         jsr draw_str

         ldx #d_crsr_h.d_petscr
         ldy #cblack
         jsr setdprops
         lda dcp_i
         cmp color_sel
         bne dcp_nosel2
         lda #"]"
         jmp dcp_bracket2
dcp_nosel2
         lda #" "
dcp_bracket2
         jsr ctxdraw
         lda #" "
         jsr ctxdraw

         inc dcp_i
         jmp dcp_loop
dcp_done
         rts

;A -> color (0-3). Sets up A/X for draw_str with that color's name.
dcp_name
         .block
         cmp #color_red
         bne dcpn_y
         lda #<colorname_red_s
         ldx #>colorname_red_s
         rts
dcpn_y   cmp #color_yellow
         bne dcpn_g
         lda #<colorname_yellow_s
         ldx #>colorname_yellow_s
         rts
dcpn_g   cmp #color_green
         bne dcpn_b
         lda #<colorname_green_s
         ldx #>colorname_green_s
         rts
dcpn_b   lda #<colorname_blue_s
         ldx #>colorname_blue_s
         rts
         .bend

dcp_i .byte 0
         .bend

;Row 5-6: Wild Draw Four challenge prompt.
draw_challenge_prompt
         .block
         lda #5
         ldx #1
         jsr goto_rc
         ldx #d_crsr_h.d_petscr
         ldy #cblack
         jsr setdprops
         lda #<challenge1_s
         ldx #>challenge1_s
         jsr draw_str

         lda #6
         ldx #1
         jsr goto_rc
         lda #<challenge2_s
         ldx #>challenge2_s
         jsr draw_str
         lda #" "
         jsr ctxdraw

         lda challenge_sel
         bne dcx_noflag1
         lda #"["
         jmp dcx_flag1
dcx_noflag1
         lda #" "
dcx_flag1
         jsr ctxdraw
         ldy #cgreen
         ldx #d_crsr_h.d_petscr
         jsr setdprops
         lda #<yes_s
         ldx #>yes_s
         jsr draw_str
         ldx #d_crsr_h.d_petscr
         ldy #cblack
         jsr setdprops
         lda challenge_sel
         bne dcx_noflag2
         lda #"]"
         jmp dcx_flag2
dcx_noflag2
         lda #" "
dcx_flag2
         jsr ctxdraw
         lda #" "
         jsr ctxdraw

         ldx #d_crsr_h.d_petscr
         ldy #cblack
         jsr setdprops
         lda challenge_sel
         beq dcx_noflag3
         lda #"["
         jmp dcx_flag3
dcx_noflag3
         lda #" "
dcx_flag3
         jsr ctxdraw
         ldy #cred
         ldx #d_crsr_h.d_petscr
         jsr setdprops
         lda #<no_s
         ldx #>no_s
         jsr draw_str
         ldx #d_crsr_h.d_petscr
         ldy #cblack
         jsr setdprops
         lda challenge_sel
         beq dcx_noflag4
         lda #"]"
         jmp dcx_flag4
dcx_noflag4
         lda #" "
dcx_flag4
         jsr ctxdraw
         rts
         .bend

draw_game_over_screen
         .block
         ldx #d_crsr_h.d_petscr
         ldy #cblack
         jsr setdprops
         lda #" "
         jsr ctxclear

         lda #8
         ldx #16
         jsr goto_rc
         ldx #d_crsr_h.d_petscr
         lda winner
         bne dgo_lose
         ldy #cgreen
         jsr setdprops
         lda #<win_s
         ldx #>win_s
         jsr draw_str
         jmp dgo_hint
dgo_lose
         ldy #cred
         jsr setdprops
         lda #<lose_s
         ldx #>lose_s
         jsr draw_str
dgo_hint
         lda #12
         ldx #6
         jsr goto_rc
         ldx #d_crsr_h.d_petscr
         ldy #cblack
         jsr setdprops
         lda #<playagain_s
         ldx #>playagain_s
         jsr draw_str
         rts
         .bend

;A -> value 0-99, drawn as 2 decimal digits (leading zero) at the
;context's current draw position.
print_num2
         .block
         sta pnval
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
         lda pnten
         clc
         adc #"0"
         jsr ctxdraw
         lda pnval
         clc
         adc #"0"
         jsr ctxdraw
         rts
pnval .byte 0
pnten .byte 0
         .bend

;A -> value 0-255, drawn as 3 decimal digits (leading zeros) at the
;context's current draw position. Only draw_count needs this (up to
;108); every other count in this game fits in two digits.
print_num3
         .block
         sta pn3val
         lda #0
         sta pn3hun
p3hloop  lda pn3val
         cmp #100
         bcc p3hdone
         sbc #100
         sta pn3val
         inc pn3hun
         jmp p3hloop
p3hdone
         lda #0
         sta pn3ten
p3tloop  lda pn3val
         cmp #10
         bcc p3tdone
         sbc #10
         sta pn3val
         inc pn3ten
         jmp p3tloop
p3tdone
         lda pn3hun
         clc
         adc #"0"
         jsr ctxdraw
         lda pn3ten
         clc
         adc #"0"
         jsr ctxdraw
         lda pn3val
         clc
         adc #"0"
         jsr ctxdraw
         rts
pn3val .byte 0
pn3hun .byte 0
pn3ten .byte 0
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
