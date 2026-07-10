;----[ engine.s - UNO game engine for C64 OS ]--------
; Platform-independent game rules, hand-ported from the shared
; cards.c/game.c/ai.c used by every cc65 port in this repo, since C64 OS
; has no C toolchain at all -- this is 6502 assembly against C64 OS's
; own SDK, not cc65.
;
; A card is packed into a single byte: (color<<4)|value. color 0-3 are
; the four suits, 4 is Wild; value 0-9 are number cards, 10-14 are
; Skip/Reverse/DrawTwo/Wild/WildDrawFour. This halves the per-card
; memory cost of the original two-byte Card struct, which matters given
; how little spare RAM an app has to work with here.
;
; Player hands are one contiguous 160-byte array (4 players x 40 slots)
; rather than 4 separate arrays, so a hand slot can be addressed with
; plain absolute-indexed addressing (player*40 + slot) instead of
; needing zero-page pointer indirection per player.

color_red    = 0
color_yellow = 1
color_green  = 2
color_blue   = 3
color_wild   = 4

val_skip    = 10
val_reverse = 11
val_draw2   = 12
val_wild    = 13
val_wild4   = 14

deck_size   = 108
num_players = 4
max_hand    = 40
g_none      = 255

;TMP doesn't support C-style << (it conflicts with </> meaning low/high
;byte of an address), so these packed-card constants are precomputed
;with plain multiplication instead of a shift.
card_wild  = (color_wild*16)+val_wild
card_wild4 = (color_wild*16)+val_wild4

;---------------------------------------
;Game state

;These arrays are explicit zero-filled bytes, not a "*= label+size"
;address-counter skip -- TMP's *= trick only advances the location
;counter, it doesn't emit any bytes, so the assembled object file came
;out as five disjoint chunks instead of one contiguous block. C64 OS's
;loader only maps what's actually present in the file, so that memory
;was never reliably allocated at all: game_new() appeared to run (the
;screen still redrew) but draw_count/hand_count read back as zero
;afterward, because the writes had nowhere real to land.
draw_pile
         .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
         .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
         .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
         .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
         .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
         .byte 0,0,0,0,0,0,0,0
draw_count    .byte 0

discard_pile
         .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
         .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
         .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
         .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
         .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
         .byte 0,0,0,0,0,0,0,0
discard_count .byte 0

player_hands
         .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
         .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
         .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
         .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
         .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
         .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
         .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
         .byte 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
hand_count
         .byte 0,0,0,0

current_player .byte 0
direction      .byte 1     ;1 or 255 ($ff, i.e. -1)

top_color .byte 0
top_value .byte 0
top_card  .byte 0

flag_win         .byte 0
winner           .byte 0
flag_uno_player  .byte g_none
flag_skip        .byte g_none
flag_reverse     .byte g_none
flag_draw_player .byte g_none
flag_draw_count  .byte 0

wd4_pending   .byte 0
wd4_player    .byte 0
wd4_victim    .byte 0
wd4_was_legal .byte 0

reject_buf
         .byte 0,0,0,0,0,0,0,0,0,0

;8-bit LCG, X'=(5*X+17) mod 256 -- full period (256) per Hull-Dobell,
;since 17 is odd and 5-1=4 is divisible by 4. Not cryptographic, just
;needs to look shuffled; stirred with each keypress's PETSCII value in
;main.s so a session isn't the same deal every time.
rng_state .byte 137

;.block/.bend scopes ALL labels declared inside it, including plain data
;bytes, not just code -- so anything shared across routines (like the
;hand slot play_card wants remove_from_hand to remove) has to live out
;here at file scope instead of inside either routine's own block.
pc_idx .byte 0

;---------------------------------------
;Small shared helpers

;Multiplies X (a player index, 0-3) by 40. Returns result in .A.
;X is preserved (only read via TXA).
mul40
         .block
         txa
         asl a
         asl a
         asl a
         sta m40_tmp
         asl a
         asl a
         clc
         adc m40_tmp
         rts
m40_tmp  .byte 0
         .bend

;Advances the RNG and returns a pseudo-random byte in .A.
rng_next
         .block
         lda rng_state
         asl a
         asl a
         clc
         adc rng_state
         adc #17
         sta rng_state
         rts
         .bend

;A -> card byte. Returns 1 in .A if legal against top_color/top_value,
;else 0.
is_legal
         .block
         sta il_card
         lsr a
         lsr a
         lsr a
         lsr a
         cmp #color_wild
         bne il_chkcolor
         lda #1
         rts
il_chkcolor
         cmp top_color
         beq il_yes
         lda il_card
         and #$0f
         cmp top_value
         beq il_yes
         lda #0
         rts
il_yes   lda #1
         rts
il_card  .byte 0
         .bend

;---------------------------------------
;Deck construction / shuffling

;Fills draw_pile[0..107] with a fresh 108-card deck in the same order
;as cards.c's deck_build(): per color, one 0, two each of 1-9, two each
;of Skip/Reverse/DrawTwo; then four Wild, four WildDrawFour.
deck_build
         .block
         ldy #0
         ldx #0
db_colorloop
         txa
         asl a
         asl a
         asl a
         asl a
         sta draw_pile,y
         iny

         lda #1
         sta db_val
db_1to9
         txa
         asl a
         asl a
         asl a
         asl a
         ora db_val
         sta draw_pile,y
         iny
         sta draw_pile,y
         iny
         inc db_val
         lda db_val
         cmp #10
         bne db_1to9

         lda #val_skip
         sta db_val
db_actions
         txa
         asl a
         asl a
         asl a
         asl a
         ora db_val
         sta draw_pile,y
         iny
         sta draw_pile,y
         iny
         inc db_val
         lda db_val
         cmp #(val_draw2+1)
         bne db_actions

         inx
         cpx #4
         bne db_colorloop

         lda #card_wild
         ldx #4
db_wild
         sta draw_pile,y
         iny
         dex
         bne db_wild

         lda #card_wild4
         ldx #4
db_wild4
         sta draw_pile,y
         iny
         dex
         bne db_wild4

         rts
db_val   .byte 0
         .bend

;A -> count. Fisher-Yates shuffles draw_pile[0..count-1] in place.
deck_shuffle
         .block
         sec
         sbc #1
         sta ds_i
ds_loop
         lda ds_i
         beq ds_done          ;stop once i==0 (i>0 keeps looping)
         bmi ds_done          ;count was 0 -- nothing to do

         lda ds_i
         clc
         adc #1
         sta ds_mod           ;mod = i+1
         jsr rng_next
ds_modloop
         cmp ds_mod
         bcc ds_modok
         sec
         sbc ds_mod
         jmp ds_modloop
ds_modok
         sta ds_j

         ldx ds_i
         lda draw_pile,x
         pha
         ldx ds_j
         lda draw_pile,x
         ldy ds_i
         sta draw_pile,y
         pla
         ldx ds_j
         sta draw_pile,x

         dec ds_i
         jmp ds_loop
ds_done
         rts
ds_i   .byte 0
ds_j   .byte 0
ds_mod .byte 0
         .bend

;---------------------------------------
;Draw pile / hand management

;If the draw pile is empty and there's more than one discard, moves all
;but the top discard back into the draw pile and reshuffles.
reshuffle_if_needed
         .block
         lda draw_count
         bne rin_done
         lda discard_count
         cmp #2
         bcc rin_done

         sec
         lda discard_count
         sbc #1
         sta rin_keep

         ldx discard_count
         dex
         lda discard_pile,x
         sta rin_top

         ldx #0
rin_copy
         cpx rin_keep
         beq rin_copydone
         lda discard_pile,x
         sta draw_pile,x
         inx
         jmp rin_copy
rin_copydone
         lda rin_keep
         sta draw_count
         jsr deck_shuffle

         lda rin_top
         sta discard_pile
         lda #1
         sta discard_count
rin_done
         rts
rin_keep .byte 0
rin_top  .byte 0
         .bend

;X -> player index. Draws the top card, appends it to that player's
;hand (if room), and returns the drawn card byte in .A.
draw_card
         .block
         stx dc_player
         jsr reshuffle_if_needed

         lda draw_count
         bne dc_have
         lda #card_wild  ;emergency fallback, deck truly empty
         rts

dc_have
         dec draw_count
         ldx draw_count
         lda draw_pile,x
         sta dc_card

         ldx dc_player
         lda hand_count,x
         cmp #max_hand
         bcs dc_skipapp
         jsr mul40             ;X=dc_player; A=player*40
         clc
         adc hand_count,x
         tay
         lda dc_card
         sta player_hands,y
         inc hand_count,x
dc_skipapp
         lda dc_card
         rts
dc_player .byte 0
dc_card   .byte 0
         .bend

;X -> player (also read from pc_idx-caller context). Removes the card
;at player_hands[player*40+pc_idx], shifting later cards down.
;pc_idx (defined below, in play_card's block) is used as the slot to
;remove -- exposed here since only play_card calls this.
remove_from_hand
         .block
         stx rfh_player
         jsr mul40
         sta rfh_base
         clc
         adc pc_idx
         sta rfh_dst
         lda rfh_dst
         clc
         adc #1
         sta rfh_src

rfh_loop
         ldx rfh_player
         lda rfh_base
         clc
         adc hand_count,x
         cmp rfh_src
         bcc rfh_done
         beq rfh_done

         ldy rfh_src
         lda player_hands,y
         ldy rfh_dst
         sta player_hands,y

         inc rfh_src
         inc rfh_dst
         jmp rfh_loop
rfh_done
         ldx rfh_player
         dec hand_count,x
         rts
rfh_player .byte 0
rfh_base   .byte 0
rfh_dst    .byte 0
rfh_src    .byte 0
         .bend

;---------------------------------------
;Turn flow

clear_flags
         .block
         lda #0
         sta flag_win
         lda #g_none
         sta flag_uno_player
         sta flag_skip
         sta flag_reverse
         sta flag_draw_player
         lda #0
         sta flag_draw_count
         rts
         .bend

;current_player = (current_player + direction) mod num_players
advance_turn
         .block
         lda current_player
         clc
         adc direction
         cmp #num_players
         bne at_chk255
         lda #0
         jmp at_store
at_chk255
         cmp #255
         bne at_store
         lda #(num_players-1)
at_store
         sta current_player
         rts
         .bend

end_turn_no_play
         .block
         jsr clear_flags
         jsr advance_turn
         rts
         .bend

;X -> hand_idx (slot within current_player's hand), .A -> chosen_color
;(0-3; ignored unless the card played is Wild/WildDrawFour).
play_card
         .block
         sta pc_chosen
         stx pc_idx

         jsr clear_flags

         lda current_player
         sta pc_cur
         lda top_color
         sta pc_old_color

         ldx pc_cur
         jsr mul40
         clc
         adc pc_idx
         tay
         lda player_hands,y
         sta pc_card

         ldx pc_cur
         jsr remove_from_hand

         lda pc_card
         lsr a
         lsr a
         lsr a
         lsr a
         cmp #color_wild
         bne pc_notwild
         lda pc_chosen
         jmp pc_gotcolor
pc_notwild
         lda pc_card
         lsr a
         lsr a
         lsr a
         lsr a
pc_gotcolor
         sta top_color
         lda pc_card
         and #$0f
         sta top_value
         lda pc_card
         sta top_card

         ldx discard_count
         cpx #deck_size
         bcs pc_skipdiscard
         lda pc_card
         sta discard_pile,x
         inc discard_count
pc_skipdiscard

         ldx pc_cur
         lda hand_count,x
         bne pc_notwon
         lda #1
         sta flag_win
         lda pc_cur
         sta winner
         rts
pc_notwon
         cmp #1
         bne pc_notuno
         lda pc_cur
         sta flag_uno_player
pc_notuno

         lda pc_card
         and #$0f
         cmp #val_skip
         beq pc_doskip
         cmp #val_reverse
         beq pc_doreverse
         cmp #val_draw2
         beq pc_dodraw2
         cmp #val_wild4
         beq pc_dowild4
         jmp pc_dodefault

pc_doskip
         jsr advance_turn
         lda current_player
         sta flag_skip
         jsr advance_turn
         rts

pc_doreverse
         lda direction
         eor #$ff
         clc
         adc #1
         sta direction
         lda pc_cur
         sta flag_reverse
         jsr advance_turn
         rts

pc_dodraw2
         jsr advance_turn
         lda current_player
         sta flag_draw_player
         lda #2
         sta flag_draw_count
         ldx current_player
         jsr draw_card
         ldx current_player
         jsr draw_card
         jsr advance_turn
         rts

pc_dowild4
         lda #0
         sta pc_hasmatch
         ldx pc_cur
         lda hand_count,x
         beq pc_w4scandone
         sta pc_loopcnt
         jsr mul40
         sta pc_scanidx
pc_w4scanloop
         ldy pc_scanidx
         lda player_hands,y
         lsr a
         lsr a
         lsr a
         lsr a
         cmp pc_old_color
         bne pc_w4scannext
         lda #1
         sta pc_hasmatch
         jmp pc_w4scandone
pc_w4scannext
         inc pc_scanidx
         dec pc_loopcnt
         bne pc_w4scanloop
pc_w4scandone
         jsr advance_turn
         lda #1
         sta wd4_pending
         lda pc_cur
         sta wd4_player
         lda current_player
         sta wd4_victim
         lda pc_hasmatch
         beq pc_wasillegal
         lda #0
         sta wd4_was_legal
         rts
pc_wasillegal
         lda #1
         sta wd4_was_legal
         rts

pc_dodefault
         jsr advance_turn
         rts

pc_chosen    .byte 0
pc_cur       .byte 0
pc_old_color .byte 0
pc_card      .byte 0
pc_hasmatch  .byte 0
pc_loopcnt   .byte 0
pc_scanidx   .byte 0
         .bend

;.A -> challenged (0 or 1). Resolves a pending Wild Draw Four challenge.
resolve_wd4
         .block
         sta rw_challenged
         jsr clear_flags

         lda rw_challenged
         beq rw_nochallenge
         lda wd4_was_legal
         bne rw_fails

         ;Challenge succeeds (play was illegal): original player draws 4.
         ldx wd4_player
         jsr draw_card
         ldx wd4_player
         jsr draw_card
         ldx wd4_player
         jsr draw_card
         ldx wd4_player
         jsr draw_card
         lda wd4_player
         sta flag_draw_player
         lda #4
         sta flag_draw_count
         jmp rw_done

rw_fails
         ;Challenge fails (play was legal): challenger draws 6, skipped.
         ldx wd4_victim
         jsr draw_card
         ldx wd4_victim
         jsr draw_card
         ldx wd4_victim
         jsr draw_card
         ldx wd4_victim
         jsr draw_card
         ldx wd4_victim
         jsr draw_card
         ldx wd4_victim
         jsr draw_card
         lda wd4_victim
         sta flag_draw_player
         lda #6
         sta flag_draw_count
         jsr advance_turn
         jmp rw_done

rw_nochallenge
         ldx wd4_victim
         jsr draw_card
         ldx wd4_victim
         jsr draw_card
         ldx wd4_victim
         jsr draw_card
         ldx wd4_victim
         jsr draw_card
         lda wd4_victim
         sta flag_draw_player
         lda #4
         sta flag_draw_count
         jsr advance_turn

rw_done
         lda #0
         sta wd4_pending
         rts
rw_challenged .byte 0
         .bend

;---------------------------------------
;New game setup

game_new
         .block
         jsr deck_build
         lda #deck_size
         sta draw_count
         jsr deck_shuffle

         lda #0
         sta hand_count
         sta hand_count+1
         sta hand_count+2
         sta hand_count+3

         lda #0
         sta gn_round
gn_roundloop
         lda #0
         sta gn_p
gn_ploop
         ldx gn_p
         jsr draw_card
         inc gn_p
         lda gn_p
         cmp #num_players
         bne gn_ploop

         inc gn_round
         lda gn_round
         cmp #7
         bne gn_roundloop

         lda #0
         sta discard_count
         sta gn_nrej

gn_findstart
         dec draw_count
         ldx draw_count
         lda draw_pile,x
         sta gn_c
         and #$0f
         cmp #10
         bcc gn_found

         ldx gn_nrej
         lda gn_c
         sta reject_buf,x
         inc gn_nrej
         jmp gn_findstart

gn_found
         lda gn_c
         sta discard_pile
         lda #1
         sta discard_count
         sta top_card         ;placeholder, overwritten below
         lda gn_c
         sta top_card
         and #$0f
         sta top_value
         lda gn_c
         lsr a
         lsr a
         lsr a
         lsr a
         sta top_color

         lda #0
         sta gn_i
gn_pushloop
         lda gn_i
         cmp gn_nrej
         beq gn_pushdone
         ldx gn_i
         lda reject_buf,x
         ldx draw_count
         sta draw_pile,x
         inc draw_count
         inc gn_i
         jmp gn_pushloop
gn_pushdone

         ;Rejected cards just got appended to draw_pile in the order
         ;they were flipped past -- meaning the LAST one rejected (the
         ;one closest to the top of the pile) would deterministically
         ;be the very first card anyone draws, every game. The shared
         ;cc65 game.c every other port uses has this same property, but
         ;it's easy to hit here since RNG entropy is weaker (no jiffy-
         ;clock busy-wait to seed from -- see README). One more shuffle
         ;pass over the whole pile fixes it without touching the
         ;shared C logic other platforms rely on.
         lda draw_count
         jsr deck_shuffle

         lda #0
         sta current_player
         lda #1
         sta direction
         lda #0
         sta wd4_pending
         jsr clear_flags
         rts

gn_round .byte 0
gn_p     .byte 0
gn_nrej  .byte 0
gn_c     .byte 0
gn_i     .byte 0
         .bend

;---------------------------------------
;CPU AI

;X -> player_idx. Returns chosen hand_idx in .A (prefers a non-wild
;legal card, falls back to a legal wild, or g_none if nothing's legal).
ai_choose_card
         .block
         stx ac_player
         lda #g_none
         sta ac_wild

         jsr mul40
         sta ac_base

         ldx ac_player
         lda hand_count,x
         sta ac_count
         lda #0
         sta ac_i

ac_loop
         lda ac_i
         cmp ac_count
         bcs ac_notfound

         lda ac_base
         clc
         adc ac_i
         tay
         lda player_hands,y
         sta ac_card

         lda ac_card
         jsr is_legal
         cmp #1
         bne ac_next

         lda ac_card
         lsr a
         lsr a
         lsr a
         lsr a
         cmp #color_wild
         bne ac_returni

         lda ac_wild
         cmp #g_none
         bne ac_next
         lda ac_i
         sta ac_wild

ac_next
         inc ac_i
         jmp ac_loop

ac_returni
         lda ac_i
         rts
ac_notfound
         lda ac_wild
         rts
ac_player .byte 0
ac_base   .byte 0
ac_count  .byte 0
ac_i      .byte 0
ac_card   .byte 0
ac_wild   .byte 0
         .bend

;X -> player_idx. Returns the color (0-3) most represented in that
;player's hand, ties favoring the lower color index.
ai_choose_color
         .block
         stx acc_player
         lda #0
         sta acc_r
         sta acc_y
         sta acc_g
         sta acc_b

         jsr mul40
         sta acc_base

         ldx acc_player
         lda hand_count,x
         sta acc_count
         lda #0
         sta acc_i

acc_loop
         lda acc_i
         cmp acc_count
         bcs acc_done

         lda acc_base
         clc
         adc acc_i
         tay
         lda player_hands,y
         lsr a
         lsr a
         lsr a
         lsr a
         cmp #color_red
         bne acc_chky
         inc acc_r
         jmp acc_next
acc_chky
         cmp #color_yellow
         bne acc_chkg
         inc acc_y
         jmp acc_next
acc_chkg
         cmp #color_green
         bne acc_chkb
         inc acc_g
         jmp acc_next
acc_chkb
         cmp #color_blue
         bne acc_next
         inc acc_b
acc_next
         inc acc_i
         jmp acc_loop

acc_done
         lda #color_red
         sta acc_best
         lda acc_r
         sta acc_bestcnt

         lda acc_y
         cmp acc_bestcnt
         bcc acc_aftery
         beq acc_aftery
         lda #color_yellow
         sta acc_best
         lda acc_y
         sta acc_bestcnt
acc_aftery
         lda acc_g
         cmp acc_bestcnt
         bcc acc_afterg
         beq acc_afterg
         lda #color_green
         sta acc_best
         lda acc_g
         sta acc_bestcnt
acc_afterg
         lda acc_b
         cmp acc_bestcnt
         bcc acc_afterb
         beq acc_afterb
         lda #color_blue
         sta acc_best
acc_afterb
         lda acc_best
         rts
acc_player  .byte 0
acc_base    .byte 0
acc_count   .byte 0
acc_i       .byte 0
acc_r       .byte 0
acc_y       .byte 0
acc_g       .byte 0
acc_b       .byte 0
acc_best    .byte 0
acc_bestcnt .byte 0
         .bend

;X -> player_idx. Returns 1 if the AI should challenge a Wild Draw Four
;(gambles on a challenge mainly when already low on cards).
ai_should_challenge_wd4
         .block
         lda hand_count,x
         cmp #5
         bcc asc_yes
         lda #0
         rts
asc_yes
         lda #1
         rts
         .bend
