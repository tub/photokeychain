;; (C) 2008 Tijs van Bakel
;;
;; This program is free software; you can redistribute it and/or modify
;; it under the terms of the GNU General Public License as published by
;; the Free Software Foundation; either version 2 of the License, or
;; (at your option) any later version.
;;
;; This program is distributed in the hope that it will be useful,
;; but WITHOUT ANY WARRANTY; without even the implied warranty of
;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;; GNU General Public License for more details.
;;
;; You should have received a copy of the GNU General Public License
;; along with this program; if not, write to the Free Software
;; Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA

;;; TETRIS for lcd photoframe
;;; smoke/ecfh, 2008

;;; BUGS:
;;; - rendering can be optimized if 8 bit mode is used
	
;; 0.1
;; + render playfield
;; + add block to playfield
;; + remove block from playfield
;; + remove line from playfield
;; + check for collission
;; + timer
;; + keyboard input
;; + check if line is full

;; 1.0
;; + score updating
;; - use bitmaps for blocks
;; - title screen
;; - level increments; run at fixed frame rate (60 fps, see gba table)

;; 2.0
;; - highscores

;; 3.0
;; - multiplayer


;; DESIGN
;; ======

;;     MAIN VARIABLES
;;     --------------
;;       - X / Y       x,y position of upper left of block
;;       - BLOCK       currently active block
;;       - FIELD       200 byte array of playfield
;;       - COLLISION 

;;     MAIN LOOP
;;     ---------
;;       - check key press
;;       - optional: move block left or right
;;       - optional: rotate block
;;       - optional: skip timer
;;       - determine if a collision is active
;;           if move_left
;;               if can_move_left
;;                   move block left              
;;           if move_right
;;               if can_move_right
;;                   move block right
;; 	  if collision
;;               reset block position to top of frame
;; 	      if line full
;; 		  remove line
;; 	  else
;; 	      remove old block from playfield
;;               add block to new position
;; 	  endif

;;       collision:
;;           if the current block touches something below

include ../include/const.asm
include ../include/font.asm

;;; CONSTANTS
        
DEFAULT_MOVE_DOWN_FRAMES	equ	20
DEFAULT_KBD_REPEAT_FRAMES       equ	10
BLOCK_SIZE_X	                equ	7
BLOCK_SIZE_Y	                equ	7
	
	        ROM

;;; MAIN ENTRY POINT
	
;;; Some code avoozl put here, better not touch it
realmain        lda     DRRL
 		pha
 		lda	DRRH
 		pha

 		lda	#$00
 		sta	DRRH

 		lda 	#$08
 		sta	DRRL

 		lda 	#$3	;map lcd controller
 		sta	DRRH

                jsr     clear_screen

	;; TODO: The simulator does not support setting
	;;	 arbitrary ranges yet.  Therefore we must use
	;;	 128x128

mainloop        jsr     read_buttons

  	        jsr     move_rotate
 	        jsr     move_left_right
 		jsr     move_down

                jsr     draw_playfield
                jsr     print_score	

	;; TODO: sleep a little here

                jsr     is_reset
	        beq     mainloop

        ;; XXX: During debugging, we want to exit, then uncomment the next 2 lines
        ;;      For a release, we want to clear the field
                jsr     reset_field
                bra     mainloop
	
		pla
		sta     DRRH
		pla
		sta     DRRL

                rts

;;; FIX_BLOCK
;;; Add a block to the playfield, clean up filled lines
fix_block       nop
	;; Check if there are lines that can be removed.
 		jsr	remove_full_lines
	
	;; Add a new block to play with
	        jsr     new_block
                rts
        
;;; RESET_FIELD
reset_field     ldx     #0
                lda     #field_height
rf_next_line    pha
                lda     #field_width
rf_next_column  pha

                lda     empty_field, x
                sta     field, x

                inx

                pla
                dec     a
                bne     rf_next_column

                pla
                dec     a
                bne     rf_next_line

                lda     #0
                sta     score

                jsr     new_block
        
                rts

;;; PRINT_FRAMECOUNT
print_framecount nop
                lda     #10
                sta     plotchar_y
                lda     #100
                sta     plotchar_x
                lda     frame_count
                pha
                lsr
                lsr
                lsr
                lsr
                lsr
                lsr
                jsr     plotbyte
                pla
                inc     a
                sta     frame_count
                rts

;;; PRINT_SCORE
print_score     lda     #10
                sta     plotchar_y
                lda     #100
                sta     plotchar_x
                lda     score
                pha
                jsr     plotbyte
                pla
                inc     a
                sta     frame_count
                rts

;; SETSCREEN_TETRIS
;; Set drawing area to 84x128 pixels
setscreen_tetris    lda	#CASET	; set x range to visible area 4..88
		sta	LCDCMD
		lda	#4
		sta	LCDDATA
		lda	#87
		sta	LCDDATA

		lda	#RASET	; set y range to visible area 4..131
		sta	LCDCMD
		lda     #4
		sta	LCDDATA
		lda	#131
		sta	LCDDATA
        
	        lda	#$2c
		sta	LCDCMD

                rts        

;; CLEAR SCREEN
;; Clear the 128x128 visible area
clear_screen    lda	#CASET	; set x range to visible area 4..131
		sta	LCDCMD
		lda	#4
		sta	LCDDATA
		lda	#131
		sta	LCDDATA

		lda	#RASET	; set y range to visible area 4..131
		sta	LCDCMD
		lda     #4
		sta	LCDDATA
		lda	#131
		sta	LCDDATA
        
	        lda	#$2c
		sta	LCDCMD

                lda     #0      ; color 0
                ldx     #128
next_line       ldy     #128
next_pixel      sta     LCDDATA   ; draw pixel
                sta     LCDDATA
                dey
                bne     next_pixel
                dex
                bne     next_line
        
                rts        

;;; NEW_BLOCK
;;; Initializes a new block and resets most variables.
new_block	lda     #0
	        sta     pos_y
	        lda     #1
	        sta     pos_x

        ;; fetch a random value in range 1..7
nb_next_random  lda     PRS     ; load a random value from the timer
                and     #7
                bne     nb_is_non_zero
                inc     a
nb_is_non_zero  sta     block_index

		rts
	
;;; IS_RESET
;;; Clears the zero flag if all 3 buttons are pressed.
is_reset        lda     button0
	        and     button1
	        and     button2
	        rts

;;; MOVE_ROTATE
;;; Rotates the block if possible
move_rotate	lda     button1
	        cmp     #0
	        beq     mr_done

                jsr     remove_current_block

	;; Cycle the rotation counter
	        lda     rotation_index
	        sta     move_rotate_tmp
	        clc
	        adc     #1
	        and     #3
                sta     rotation_index

	;; Check if there is a collision
		jsr     collide
	        lda     is_collision
	        cmp     #0
	        beq     mr_draw

	;; Reset the rotation, because we have a collision on our hands
	        lda     move_rotate_tmp
	        sta     rotation_index
        
mr_draw         jsr     add_current_block
        
mr_done		rts
	
;;; MOVE_DOWN
;;; Moves down the block, based on a timer.
;;; If both left and right buttons (button0 and button2) are pressed,
;;; the timer is reset directly and the block moves down faster.
move_down       lda     button0 ; do not wait for timer if both left and right buttons are pressed simultaneously
	        and     button2
	        cmp     #0
	        beq     md_no_button

	        lda     #1
	        sta     move_down_frames

md_no_button    lda     move_down_frames ; wait for timer
	        sec
	        sbc     #1
	        sta     move_down_frames
	        cmp     #0
                bne     md_done
		lda     #DEFAULT_MOVE_DOWN_FRAMES
		sta     move_down_frames

        ;; store the current position
                lda     pos_y
                sta     move_down_tmp
        
                jsr     remove_current_block
	
	        lda	pos_y   ; move the block down 1 row
            	clc
	        adc     #1
	        sta     pos_y

        ;; check if we have a colission
                jsr     collide
	        lda     is_collision
	        cmp     #0
	        beq     md_no_collision
        
        ;; there is a collision, fix the block
                lda     move_down_tmp ; we have a colission
	        sta     pos_y
                jsr     add_current_block
                jsr     fix_block
                bra     md_done

md_no_collision jsr     add_current_block
        
md_done    	rts

;;; MOVE_LEFT_RIGHT
move_left_right jsr     remove_current_block
                lda     pos_x
	        sta     move_lr_tmp

		lda     button0
	        cmp     #0
	        beq     mlr_no_left
	
	;; Move to left
		lda	pos_x
	        sec
	        sbc     #1
	        sta     pos_x

mlr_no_left     lda     button2
	        cmp     #0
	        beq     mlr_no_right
	
	;; Move to right
		lda	pos_x
	        clc
	        adc     #1
	        sta     pos_x
	
mlr_no_right    jsr     collide
	        lda     is_collision
	        cmp     #0
	        beq     mlr_done

	;; Reset the position, because we have a collision on our hands
	        lda     move_lr_tmp
	        sta     pos_x

mlr_done        jsr     add_current_block
                rts
	
;;; READ_BUTTONS
;;; TODO: On keyboard change => directly
;;;       otherwise, repeat after counter
read_buttons    lda     #0
	        sta     button0
	        sta     button1
	        sta     button2

                lda     0
                and     #7
                sta     pa_state
                cmp     prev_pa_state
                beq     rb_no_pa_change

		lda     #DEFAULT_KBD_REPEAT_FRAMES
		sta     kbd_repeat_frames
                bra     rb_change

rb_no_pa_change lda     kbd_repeat_frames ; wait for timer for keyboard repeat
                sec
                sbc     #1
	        sta     kbd_repeat_frames
	        cmp     #0
                bne     rb_no_change

		lda     #DEFAULT_KBD_REPEAT_FRAMES
		sta     kbd_repeat_frames
        
rb_change       lda     #1
	        sta     button0
	        sta     button1
	        sta     button2

                lda     pa_state
                sta     prev_pa_state
	        tax
	        and     #1
	        beq     no_button0
	        lda     #0
	        sta     button0
no_button0      txa
	        and     #2
	        beq     no_button1
	        lda     #0
	        sta     button1
no_button1      txa
	        and     #4
	        beq     no_button2
	        lda     #0
	        sta     button2
no_button2      rts
rb_no_change    rts        
	
;;; WILL_COLLIDE
;;; Tests if there would be a collision if the block would fall another time
will_collide    lda     pos_x	;save x,y
	        pha
	        lda     pos_y
	        pha
	        jsr     move_down
	        jsr     collide
	        pla		;reset x,y
	        sta     pos_y
	        pla
	        sta     pos_x
		rts
	
;;; DECODE_BLOCK
;;; In:  block_index    The value of the block (1..7)
;;;      rotation_index The rotation of the block (0..3)
;;;      packed_shapes  The format of the blocks
;;; Out: block_shape    The block in 4x4 byte format
decode_block    lda     block_index
	        sec
	        sbc     #1
                asl 
                asl 
                asl 
                asl 
                asl
	        sta     decode_tmp

	        lda     rotation_index
	        asl
	        asl
	        asl
	        clc
	        adc     decode_tmp
	
	        tay		; y = index in packed_shapes table
	        ldx     #0      ; x = index in block_shape table

	        lda     #8
decode_nexty    pha

        	lda     packed_shapes, y
	        lsr
	        lsr
	        lsr
	        lsr
	        sta     block_shape, x
	        inx
	
	        lda     packed_shapes, y
	        and     #$f
	        sta     block_shape, x
	        inx

	        iny
	
	        pla
	        dec     a
	        bne     decode_nexty
	 
	
	        rts

;;; REMOVE_BLOCK
remove_current_block	lda	#0
		sta	draw_color
		jsr	draw_block
                rts

;;; DRAW_BLOCK
add_current_block      lda     block_index
	        sta     draw_color
	        jsr     draw_block
                rts
        
;;; DRAW_BLOCK
;;; Updates the field with the current block.
;;; In:  draw_color The color to update the field with (0..7)
;;;      pos_x      The x position to add the block at (0..9)
;;;      pos_y      The y position to add the block at (0..19)
;;; Out: field      The field is updated with the given block
draw_block      jsr     decode_block
	        jsr     get_field_pos ; y = field index
	        ldx     #0      ; x = block index

	        lda     #4
draw_block_y    pha
	
	        lda     #4
draw_block_x    pha

	        lda     block_shape, x
	        cmp     #0
	        beq     draw_block_skip

	        lda	draw_color
                ora     #$80    ; set dirty flag
	        sta     field, y

draw_block_skip inx
	        iny

	        pla
	        dec	a
	        bne     draw_block_x

	        tya		; add 12-4 = 8 to field index
	        clc
	        adc     #8
	        tay
	
	        pla
	        dec     a
	        bne     draw_block_y
	
	        rts

;;; COLLIDE
;;; Checks if the current block collides with the playfield.
;;; In:  pos_x      The x position to add the block at (0..9)
;;;      pos_y      The y position to add the block at (0..19)
;;; Out: collision  1 in case of a collision, 0 otherwise
collide         jsr     decode_block

	        jsr     get_field_pos ; y = field index
	        ldx     #0	; x = block index

	        lda     #0
	        sta     is_collision

	        lda     #4
coll_block_y    pha
	
	        lda     #4
coll_block_x    pha

	        lda     block_shape, x
	        cmp     #0
	        beq     coll_block_skip

	        lda     field, y
                and     #$0f    ; mask out the dirty flag
                cmp     #0
	        beq     coll_block_skip

	        lda     #1
	        sta     is_collision

coll_block_skip inx
	        iny

	        pla
	        dec	a
	        bne     coll_block_x

	        tya		; add 12-4 = 8 to field index
	        clc
	        adc     #8
	        tay
	
	        pla
	        dec     a
	        bne     coll_block_y
	
	        rts

;;; GET_FIELD_POS
;;; Calculates the field position (pos_x + pos_y * 12)
;;; In:  pos_x      The x position (0..9)
;;;      pos_y      The y position (0..19)
;;; Out: Y          The field index (pos_x + pos_y * 12)
get_field_pos   lda     pos_y
 		jsr     mul12
	        adc     pos_x
	        tay		; y = field index
        	rts

;;; MUL12
;;; Multiplies accumulator with 12
mul12           asl
	        asl
	        sta     mul12_tmp
	        asl
	        clc
	        adc     mul12_tmp
	        rts

;;; DRAW_PLAYFIELD
;;; Draws the 18x10 blocks playfield
;;; 
;;; Only the fields with the dirty flag ($80) set are redrawn.
;;; After drawing, the dirty flag is cleared.
;;; 
;;; In:  field    the playfield (18x10 elements)
;;;      clut_hi  high 8 bit part of the color lookup table
;;;      clut_lo  low 8 bit part of the color lookup table
;;; Out: The LCD screen is updated
draw_playfield  lda     #0
                sta     draw_field_y
                sta     draw_field_x
        
                ldy     #0
                ldx     #18
        
dp_next_row     phx
                ldx     #12

                lda     draw_field_y
                clc
                adc     #BLOCK_SIZE_Y
                sta     draw_field_y
                lda     #0
                sta     draw_field_x
        
dp_next_column  phx

                lda     draw_field_x
                clc
                adc     #BLOCK_SIZE_X
                sta     draw_field_x

                lda     field, y
                iny
        
                phy
                jsr     draw_subblock
                ply

                plx
                dex
                bne     dp_next_column

                plx
                dex
                bne     dp_next_row

                rts

;;; DRAW_SUBBLOCK
;;; Draws one part of a tetris block
;;; In: a            block index to render (1..7)
;;;     draw_field_x x position on screen
;;;     draw_field_y y position on screen
;;;     BLOCK_WIDTH  width of a block in pixels
;;;     BLOCK_HEIGHT height of a block in pixels
draw_subblock   pha             ;store block index
        
                lda     #CASET
                sta     LCDCMD
                lda     draw_field_x
                sta     LCDDATA
                clc
                adc     #BLOCK_SIZE_X-1
                sta     LCDDATA

                lda     #RASET
                sta     LCDCMD
                lda     draw_field_y
                sta     LCDDATA
                clc
                adc     #BLOCK_SIZE_Y-1
                sta     LCDDATA

                lda     #RAMWR
                sta     LCDCMD

                pla             ;restore block index
                and     #$0f     ;mask out dirty flag
                tax

                lda     clut_lo, x ; a = high color value
                ldy     clut_hi, x ; y = low color value
        
        ;; non optimized drawing
;;                 ldx     #BLOCK_SIZE_X * BLOCK_SIZE_Y
;; ds_next_pixel   sta     LCDDATA
;;                 sty     LCDDATA
;;                 dex
;;                 bne     ds_next_pixel

        ;; optimized drawing, assume that block width is 7 pixels
                ldx     #BLOCK_SIZE_Y
ds_next_pixel   sta     LCDDATA ;pixel 0
                sty     LCDDATA
                sta     LCDDATA ;pixel 1
                sty     LCDDATA
                sta     LCDDATA ;pixel 2
                sty     LCDDATA
                sta     LCDDATA ;pixel 3
                sty     LCDDATA
                sta     LCDDATA ;pixel 4
                sty     LCDDATA
                sta     LCDDATA ;pixel 5
                sty     LCDDATA
                sta     LCDDATA ;pixel 6
                sty     LCDDATA
                dex
                bne     ds_next_pixel

                rts        


;;; REMOVE_FULL_LINES
remove_full_lines	lda	#17
rfl_next_line	pha

		sta	line_index
rfl_check_again	jsr	check_full_line

		lda	is_full
		cmp	#0
		beq	rfl_not_full

                inc     score
 		jsr     remove_line
		bra	rfl_check_again
	
rfl_not_full	pla
		dec	a
		bne	rfl_next_line
	
		rts

;;; CHECK_FULL_LINE
;;; In:  field            The playfield
;;;      line_index       Index of the line to check (0..17)
;;;      is_full          1 if the line is full, 0 otherwise
check_full_line lda     #1
	        sta     is_full

	        lda     line_index
		jsr	mul12
                tay		; y = index in field

	        ldx     #12
cfl_next_column lda     field, y
                and     #$0f    ; mask out the dirty flag
	        cmp     #0
	        bne     cfl_not_empty

	        lda     #0
	        sta     is_full
	
cfl_not_empty	iny
	
	        dex
	        bne     cfl_next_column
		rts

;;; REMOVE_LINE
;;; TODO: Implement
;;; In:  line_index  The line to remove
;;;      field       The field to update
;;; Out: field       The field after removal and compacting
remove_line	ldx 	line_index
rl_next_line	phx

	        txa
		jsr	mul12
	        tay

	        ldx     #12
rl_next_column  lda     field-12, y
	        sta     field, y
		iny
	        dex
	        bne     rl_next_column

		plx
	        dex
	        bne     rl_next_line
	
		rts

remove_line2	lda 	line_index
		jsr	mul12
	        tay
		lda	#$80    ; clear the line, set the dirty flag
		sta	field, y
		rts

;;; CONSTANT DATA
        
                ROM

;;; Color lookup table
clut_lo		db	$00	; 0 X black
		db	$D0	; 1 I red
		db	$FF	; 2 J white
		db	$E0	; 3 L magenta
		db	$01	; 4 O blue
		db	$2D	; 5 T green
		db	$BC	; 6 S brown
		db	$4C	; 7 Z cyan
	        db      $0F	; 8 border
clut_hi		db	$80	; 0 X black
		db	$00	; 1 I red
		db	$58	; 2 J white
		db	$31	; 3 L magenta
		db	$B8	; 4 O blue
		db	$87  	; 5 T green
		db	$6D	; 6 S brown
		db	$F4	; 7 Z cyan
	        db      $0F     ; 8 border

;;; Block shapes (4 rotations per block, 7 blocks, 4x4, 4 bit per element => 224 bytes)
packed_shapes	db      $00,$00 ; 1 I0
	        db      $00,$00
	        db      $11,$11
	        db      $00,$00
	
	        db      $01,$00 ;   I1
	        db      $01,$00 
	        db      $01,$00
	        db      $01,$00
	
	        db      $00,$00 ;   I2
	        db      $00,$00 
	        db      $11,$11
	        db      $00,$00
	
	        db      $01,$00 ;   I3
	        db      $01,$00 
	        db      $01,$00
	        db      $01,$00
	
	        db	$00,$00	; 2 J0
           	db	$11,$10
           	db	$00,$10
           	db	$00,$00
	
	        db	$01,$00	;   J1
           	db	$01,$00
           	db	$11,$00
           	db	$00,$00
	
	        db	$10,$00	;   J2
           	db	$11,$10
           	db	$00,$00
           	db	$00,$00
	
	        db	$01,$10	;   J3
           	db	$01,$00
           	db	$01,$00
           	db	$00,$00

	        db      $00,$00 ; 3 L0
	        db      $11,$10
	        db      $10,$00
	        db      $00,$00

	        db      $11,$00 ;   L1
	        db      $01,$00
	        db      $01,$00
	        db      $00,$00

	        db      $00,$10 ;   L2
	        db      $11,$10
	        db      $00,$00
	        db      $00,$00

	        db      $01,$00 ;   L3
	        db      $01,$00
	        db      $01,$10
	        db      $00,$00

	        db      $11,$00 ; 4 O0
	        db      $11,$00
	        db      $00,$00
	        db      $00,$00

	        db      $11,$00 ;   O1
	        db      $11,$00
	        db      $00,$00
	        db      $00,$00

	        db      $11,$00 ;   O2
	        db      $11,$00
	        db      $00,$00
	        db      $00,$00

	        db      $11,$00 ;   O3
	        db      $11,$00
	        db      $00,$00
	        db      $00,$00

	        db      $00,$00 ; 5 T0
	        db      $11,$10
	        db      $01,$00
	        db      $00,$00

	        db      $01,$00 ;   T1
	        db      $11,$00
	        db      $01,$00
	        db      $00,$00

	        db      $01,$00 ;   T2
	        db      $11,$10
	        db      $00,$00
	        db      $00,$00

	        db      $01,$00 ;   T3
	        db      $01,$10
	        db      $01,$00
	        db      $00,$00

	        db      $00,$00 ; 6 S0
	        db      $01,$10
	        db      $11,$00
	        db      $00,$00

	        db      $10,$00 ;   S1
	        db      $11,$00
	        db      $01,$00
	        db      $00,$00

	        db      $00,$00 ;   S2
	        db      $01,$10
	        db      $11,$00
	        db      $00,$00

	        db      $10,$00 ;   S3
	        db      $11,$00
	        db      $01,$00
	        db      $00,$00

	        db      $00,$00 ; 7 Z0
	        db      $11,$00
	        db      $01,$10
	        db      $00,$00

	        db      $01,$00 ;   Z1
	        db      $11,$00
	        db      $10,$00
	        db      $00,$00

	        db      $00,$00 ;   Z2
	        db      $11,$00
	        db      $01,$10
	        db      $00,$00

	        db      $01,$00 ;   Z3
	        db      $11,$00
	        db      $10,$00
	        db      $00,$00

empty_field     db	$88,$80,$80,$80,$80,$80,$80,$80,$80,$80,$80,$88
		db	$88,$80,$80,$80,$80,$80,$80,$80,$80,$80,$80,$88
		db	$88,$80,$80,$80,$80,$80,$80,$80,$80,$80,$80,$88
		db	$88,$80,$80,$80,$80,$80,$80,$80,$80,$80,$80,$88
		db	$88,$80,$80,$80,$80,$80,$80,$80,$80,$80,$80,$88
		db	$88,$80,$80,$80,$80,$80,$80,$80,$80,$80,$80,$88
		db	$88,$80,$80,$80,$80,$80,$80,$80,$80,$80,$80,$88
		db	$88,$80,$80,$80,$80,$80,$80,$80,$80,$80,$80,$88
		db	$88,$80,$80,$80,$80,$80,$80,$80,$80,$80,$80,$88
		db	$88,$80,$80,$80,$80,$80,$80,$80,$80,$80,$80,$88
		db	$88,$80,$80,$80,$80,$80,$80,$80,$80,$80,$80,$88
		db	$88,$80,$80,$80,$80,$80,$80,$80,$80,$80,$80,$88
		db	$88,$80,$80,$80,$80,$80,$80,$80,$80,$80,$80,$88
		db	$88,$80,$80,$80,$80,$80,$80,$80,$80,$80,$80,$88
		db	$88,$80,$80,$80,$80,$80,$80,$80,$80,$80,$80,$88
		db	$88,$80,$80,$80,$80,$80,$80,$80,$80,$80,$80,$88
		db	$88,$80,$80,$80,$80,$80,$80,$80,$80,$80,$80,$88
		db	$88,$80,$80,$80,$80,$80,$80,$80,$80,$80,$80,$88
		db	$88,$88,$88,$88,$88,$88,$88,$88,$88,$88,$88,$88

;;; VARIABLE DATA
	        RAM
	
draw_color	db 	$2

;;; Decoded block 4x4 bytes
block_shape	db	$00,$00,$00,$00
	        db      $00,$00,$00,$00
	        db      $00,$00,$00,$00
	        db      $00,$00,$00,$00

block_index	db      1 	; current block (1..7)
rotation_index  db      1	; rotation (0..3)
pos_x           db      1	; block x position in playfield (0,0 is top left)
pos_y           db      9	; block y position in playfield

mul12_tmp	db	$00
move_lr_tmp	db	$00
move_down_tmp	db	$00

is_collision	db      $00	; 1 if collision detected, 0 otherwise
	
line_index      db      $0	; line index, used in various algorithms (0..17)
	
is_full         db      $0	; 1 if line is full, 0 otherwise
	
button0         db      $0	; button 0 (left) 1 if pushed, 0 otherwise
button1         db      $0	; button 1 (center) 1 if pushed, 0 otherwise
button2         db      $0	; button 2 (right) 1 if pushed, 0 otherwise

move_down_frames db     DEFAULT_MOVE_DOWN_FRAMES
kbd_repeat_frames db     DEFAULT_KBD_REPEAT_FRAMES

decode_tmp	db      0       ; temporary variable used by decode_block
move_rotate_tmp	db	0	; temporary variable used in move_rotate

;;; Tetris playfield
;;; The actual playfield is 10*18 blocks.  We use a border to make
;;; collision detection simpler.  Hence the actual size is 12*20 = 240
;;; bytes.  This still fits in one page, so we are happy.
;;; Bit $80 is set if we must redraw the field (dirty flag)
empty_line	db	$88,$80,$80,$80,$80,$80,$80,$80,$80,$80,$80,$88
field           db	$88,$80,$80,$80,$80,$80,$80,$80,$80,$80,$80,$88
		db	$88,$80,$80,$80,$80,$80,$80,$80,$80,$80,$80,$88
		db	$88,$80,$80,$80,$80,$80,$80,$80,$80,$80,$80,$88
		db	$88,$80,$80,$80,$80,$80,$80,$80,$80,$80,$80,$88
		db	$88,$80,$80,$80,$80,$80,$80,$80,$80,$80,$80,$88
		db	$88,$80,$80,$80,$80,$80,$80,$80,$80,$80,$80,$88
		db	$88,$80,$80,$80,$80,$80,$80,$80,$80,$80,$80,$88
		db	$88,$80,$80,$80,$80,$80,$80,$80,$80,$80,$80,$88
		db	$88,$80,$80,$80,$80,$80,$80,$80,$80,$80,$80,$88
		db	$88,$80,$80,$80,$80,$80,$80,$80,$80,$80,$80,$88
		db	$88,$80,$80,$80,$80,$80,$80,$80,$80,$80,$80,$88
		db	$88,$80,$80,$80,$80,$80,$80,$80,$80,$80,$80,$88
		db	$88,$80,$80,$80,$80,$80,$80,$83,$80,$80,$80,$88
		db	$88,$80,$80,$80,$80,$80,$81,$83,$83,$85,$85,$88
		db	$88,$81,$80,$84,$84,$84,$81,$83,$82,$85,$85,$88
		db	$88,$81,$82,$84,$83,$84,$81,$82,$82,$82,$86,$88
		db	$88,$81,$82,$82,$83,$84,$81,$85,$85,$86,$86,$88
		db	$88,$81,$82,$83,$83,$84,$84,$85,$85,$86,$80,$88
		db	$88,$88,$88,$88,$88,$88,$88,$88,$88,$88,$88,$88

field_width     equ     12
field_height    equ     19        
        
frame_count     db      0

score           db      0

draw_field_x    db      0
draw_field_y    db      0

prev_pa_state   db      0
pa_state        db      0
        
