    CPU 65c02
    OUTPUT HEX
    INCLUDE spec
    * = EMPTY_AT+$4000
;The routine in the existing firmware is patched to jump here if the
;routine that discerns the address that's written to fails.
;This way, we can splice our own check inthere too.

;check magic write to address 4200
start	lda CMP_VAR1
	cmp #$21
	bne nomagic
	lda CMP_VAR2
	cmp #$00
	bne nomagic
	bra gotcha
;Nope? Do what the original routine did & bail out.
nomagic lda #$ff
	ldx #$ff
	jmp PATCH_AT+$4004
;ack usb wossname
gotcha  lda #$04
	sta $73

;Push registers	
	lda $34
	pha
	lda $35
	pha
	
;The LCD of this device isn't connected to the perfectly good internal LCD
;subsystem, but is an external one with its own controller. It seems to be
;connected to the dataspace when DRRH==3. Commands to it go to $8000, data
;to $c000. The controller is PCF8833-compatible btw, same type as some
;Nokia 6100s (and other Nokias) have.

;select lcd
	lda #$3
	sta $35

;reset addr
	lda #$2b
	sta $8000
	lda #$4
	sta $c000
	lda #$83
	sta $c000

	lda #$2a
	sta $8000
	lda #$4
	sta $c000
	lda #$83
	sta $c000

;go store data!
	lda #$2c
	sta $8000


	stz LEN0
;wait for usb packet
waitpacket lda $73
	and #$4
	beq waitpacket

;copy packet to framebuff
	ldx #$0
copyloop lda $200,x
	sta $c000
	inx
	;cpx #$40 ;wrongly assembled by crasm, I'll do it manually:
	db $e0,$40
	bne copyloop


;subtract 0x40 from 37A:37D.
;Damn, this is way easier on an ARM :P
	sec
	lda LEN0
	sbc #$40
	sta LEN0
	lda LEN1
	sbc #$0
	sta LEN1
	lda LEN2
	sbc #$0
	sta LEN2
	lda LEN3
	sbc #$0
	sta LEN3

;ack
	lda #$04
	sta $73

;check for done-ness
	lda LEN3
	ora LEN2
	ora LEN1
	ora LEN0
	bne waitpacket
		
;restore registers
	pla
	sta $35
	pla
	sta $34

;send ack
	lda #$00
	jsr SEND_CSW+0x4000

;and return as a winner :)
	lda #$ff
	ldx #$ff
	jmp PATCH_AT+$4004
