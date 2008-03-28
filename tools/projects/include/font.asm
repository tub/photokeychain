;;; PLOTBYTE
;;; In: plotchar_x     Position on screen (in pixel base)
;;;     plotchar_y     Position on screen (in pixel base)
;;;     a              The value to plot
plotbyte        pha

                pha
                lsr
                lsr
                lsr
                lsr
                sta     plotchar_char
                jsr     plotchar
                pla

                and     #$0f
                sta     plotchar_char
                lda     plotchar_x
                pha
                clc
                adc     #8
                sta     plotchar_x
                jsr     plotchar
                pla
                sta     plotchar_x
        
                pla
                rts

;;; PLOTCHAR
;;; In: plotchar_x     Position on screen (in pixel base)
;;;     plotchar_y     Position on screen (in pixel base)
;;;     plotchar_char  Value to print 0..F
plotchar        phx
                phy
                pha

	        lda     plotchar_x ; set column range
                ldx     #CASET
                stx     LCDCMD
	        sta     LCDDATA
                clc
                adc     #7
   	        sta     LCDDATA

	        lda     plotchar_y ; set row range
                ldx     #RASET
                stx     LCDCMD
                sta     LCDDATA
                clc
                adc     #7
                sta     LCDDATA
        
                lda     #RAMWR ; start lcd write
                sta     $8000

                lda     plotchar_char
                asl
                asl
                asl
                tax ; x = index in character table
        
                lda     #8
pc_nextline     pha        

                lda     #$80
pc_nextpixel    pha
       
                ldy     #$00
                and     char0, x
                cmp     #$0
                beq     pc_no_pixel
                ldy     #$ff
pc_no_pixel     sty     $c000
                sty     $c000

                pla
                lsr
                bne     pc_nextpixel

                inx
                pla
                dec     a
                bne     pc_nextline

                pla
                ply
                plx

                rts

        ROM

char0   db 01111100b ;0
        db 11000110b
        db 11000110b
        db 11010110b
        db 11000110b
        db 11000110b
        db 01111100b
        db 00000000b

char1   db 00011000b ;1
        db 00111000b
        db 00011000b
        db 00011000b
        db 00011000b
        db 00011000b
        db 00111100b
        db 00000000b

char2   db 01111100b ;2
        db 11000110b
        db 00001100b
        db 00111000b
        db 01100000b
        db 11000000b
        db 11111110b
        db 00000000b

char3   db 01111100b ;3
        db 11000110b
        db 00000110b
        db 00001100b
        db 00000110b
        db 11000110b
        db 01111100b
        db 00000000b

char4   db 00001100b ;4
        db 00111100b
        db 01101100b
        db 11001100b
        db 11111110b
        db 00001100b
        db 00001100b
        db 00000000b

char5   db 11111110b ;5
        db 11000000b
        db 11000000b
        db 11111100b
        db 00000110b
        db 00000110b
        db 11111100b
        db 00000000b

char6   db 01111100b ;6
        db 11000000b
        db 11000000b
        db 11111100b
        db 11000110b
        db 11000110b
        db 01111100b
        db 00000000b

char7   db 11111110b ;7
        db 00000110b
        db 00000110b
        db 00001100b
        db 00011000b
        db 00011000b
        db 00011000b
        db 00000000b

char8   db 01111100b ;8
        db 11000110b
        db 11000110b
        db 01111100b
        db 11000110b
        db 11000110b
        db 01111100b
        db 00000000b

char9   db 01111100b ;9
        db 11000110b
        db 11000110b
        db 01111110b
        db 00000110b
        db 11000110b
        db 01111100b
        db 00000000b

charA   db 01111100b ;A
        db 11000110b
        db 11000110b
        db 11000110b
        db 11111110b
        db 11000110b
        db 11000110b
        db 00000000b

charB   db 11111100b ;B
        db 11000110b
        db 11000110b
        db 11111100b
        db 11000110b
        db 11000110b
        db 11111100b
        db 00000000b

charC   db 01111100b ;C
        db 11000110b
        db 11000000b
        db 11000000b
        db 11000000b
        db 11000110b
        db 01111100b
        db 00000000b

charD   db 11111100b ;D
        db 11000110b
        db 11000110b
        db 11000110b
        db 11000110b
        db 11000110b
        db 11111100b
        db 00000000b

charE   db 11111110b ;E
        db 11000000b
        db 11000000b
        db 11110000b
        db 11000000b
        db 11000000b
        db 11111110b
        db 00000000b

charF   db 11111110b ;F
        db 11000000b
        db 11000000b
        db 11110000b
        db 11000000b
        db 11000000b
        db 11000000b
        db 00000000b

        RAM

plotchar_x      db      0 ; position to plot character (in pixels)
plotchar_y      db      0 ; position to plot character (in pixels)
plotchar_char   db      0 ; character to plot 0..F
