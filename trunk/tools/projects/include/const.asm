;;; Useful constants

;;; ST2203U I/O map
        
DRRL    equ     $34             ; DRR bank register low part
DRRH    equ     $35             ; DRR bank register high part

PRS     equ     $29             ; Prescaler timer counter

LCDCMD  equ     $8000           ; LCD command port
LCDDATA equ     $C000           ; LCD read/write port

;;; LCD Commands

CASET   equ     $2A             ; column set
RASET   equ     $2B             ; row set
RAMWR   equ     $2C

MADCTR  equ $36