;;; plasma.asm - a simple demo effect
;;; smoke/ecfh, 2008
        
                RAM
include ../include/const.asm
include ../include/font.asm

PP_TMP          equ     $84 ; zero page temporary variable
                
;;; CODE main entry point
realmain        jsr     init_screen
                jsr     reset_screen

next_frame      jsr     paint_plasma
                
                lda     pp_frame_index ; increment frame index
                inc     a
                sta     pp_frame_index

                lda     $0
                and     #$1
                bne     next_frame
                rts

;;; PAINT_PLASMA
paint_plasma    lda     0
                ldx     #128
pp_next_line    phx

                ; f(y) = sin(sin(sin(y - f) + f) / 2) + f) / 2) - y - f*2)
                stx     pp_y_index

                txa
                sec
                sbc     pp_frame_index
                tax
                
                lda     sindata, x
                adc     pp_frame_index
                tax
                lda     sindata, x
                lsr
                adc     pp_frame_index
                lsr
                sbc     pp_y_index
                sbc     pp_frame_index
                sbc     pp_frame_index
                tax
                lda     sindata, x
                
                sta     pp_y_value
                sta     PP_TMP

                adc     pp_frame_index

                ; inner loop
                ldy     #128
pp_next_column  tya

                ; color = sin(x) + f(y)
test            lda     sindata, y
                adc     PP_TMP
                                
                tax
                lda     pp_clut_hi, x
                sta     LCDDATA
                lda     pp_clut_lo, x
                sta     LCDDATA
                
                dey
                bne     pp_next_column

                ldx     pp_y_value
                inx
                stx     pp_y_value
                                                
                plx
                dex
                bne     pp_next_line

;;; RESET_SCREEN
init_screen     lda     #$3
                sta     $35
                
reset_screen    lda	#CASET	; set x range to visible area 4..131
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
                
                rts                

;;; DATA variables

                RAM
                
pp_frame_index  db      0
                
pp_y_value      db      0
pp_y_index      db      0
pp_x_index      db      0

                ROM
                
sindata         db 127,130,133,136,139,143,146,149,152,155,158,161,164,167,170,173
                db 176,179,182,184,187,190,193,195,198,200,203,205,208,210,213,215
                db 217,219,221,224,226,228,229,231,233,235,236,238,239,241,242,244
                db 245,246,247,248,249,250,251,251,252,253,253,254,254,254,254,254
                db 255,254,254,254,254,254,253,253,252,251,251,250,249,248,247,246
                db 245,244,242,241,239,238,236,235,233,231,229,228,226,224,221,219
                db 217,215,213,210,208,205,203,200,198,195,193,190,187,184,182,179
                db 176,173,170,167,164,161,158,155,152,149,146,143,139,136,133,130
                db 127,124,121,118,115,111,108,105,102,99,96,93,90,87,84,81
                db 78,75,72,70,67,64,61,59,56,54,51,49,46,44,41,39
                db 37,35,33,30,28,26,25,23,21,19,18,16,15,13,12,10
                db 9,8,7,6,5,4,3,3,2,1,1,0,0,0,0,0
                db 0,0,0,0,0,0,1,1,2,3,3,4,5,6,7,8
                db 9,10,12,13,15,16,18,19,21,23,25,26,28,30,33,35
                db 37,39,41,44,46,49,51,54,56,59,61,64,67,70,72,75
                db 78,81,84,87,90,93,96,99,102,105,108,111,115,118,121,124

;;; DATA constants

                include clut.inc
