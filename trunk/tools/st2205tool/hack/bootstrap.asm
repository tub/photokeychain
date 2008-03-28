    CPU 65c02
    OUTPUT HEX
    * = $1000
;The routine in the existing firmware is patched to jump here if the
;routine that discerns the address that's written to fails.
;This way, we can splice our own check inthere too.

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

  lda #$f0
  sta $c000
  sta $c000
  sta $c000
  sta $c000
  sta $c000
  sta $c000
  sta $c000
  sta $c000
  lda #$0f
  sta $c000
  sta $c000
  sta $c000
  sta $c000
  sta $c000
  sta $c000
  sta $c000
  sta $c000

	pla
	sta $35
	pla
	sta $34

  rts