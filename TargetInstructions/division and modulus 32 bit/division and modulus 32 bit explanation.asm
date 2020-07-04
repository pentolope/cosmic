

{ ; div_u_u

POP2 %1 %2 ;LEFT
POP2 %3 %4 ;RIGHT


; signed conversions would happen here



; extended area for dividend is %5 %6 (which will eventually be the remainder)
; area for quotient is %7 %8



BL1_ %5 $00
BL1_ %6 $00
BL1_ %7 $00
BL1_ %8 $00
RL1_ %C $FFFF


{ ; do this 32 times

{ ; this shifts the dividend over
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
}





{ ; this attempts subtraction into temporary space
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
; subtraction done, carry is in %D
}

{ ; insert appropiate result into dividend
ADDN %9 %C %D
; now, %9 contains FFFF if %D==0 and 0000 if %D==1
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
}


{ ; this shifts the quotient over and inserts carry bit
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %8 %8 %8
OR__ %8 %8 %D
}



}


; signed convertions would be here







; push quotient
PU2_ %8 %7



}





















{ ; div_s_s


POP2 %1 %2 ;LEFT
POP2 %3 %4 ;RIGHT


; signed conversions would happen here

; at end of sign convertion, A will hold the xor of sign bits (in all bits), then it will push it to stack and then pop it after
; for the modulus, it is slightly different in that A will actually just be the left sign


RL1_ %C $FFFF
BL1_ %D $80
BSWP %D %D

AND_ %5 %2 %D
ADDC %5 %5 %D
ADDN %5 %F %C
XOR_ %5 %5 %C
MOV_ %A %5
MOV_ %6 %F
; %5 now contains the sign bit of left in all bits
; %6 now contains the sign bit in the ones bit
XOR_ %1 %1 %5
XOR_ %2 %2 %5
ADDC %1 %1 %6
ADDN %2 %2 %F



AND_ %5 %4 %D
ADDC %5 %5 %D
ADDN %5 %F %C
XOR_ %5 %5 %C
XOR_ %A %A %5
MOV_ %6 %F
; %5 now contains the sign bit of right in all bits
; %6 now contains the sign bit of right in the ones bit
XOR_ %3 %3 %5
XOR_ %4 %4 %5
ADDC %3 %3 %6
ADDN %4 %4 %F


; push the sign xor
PU1_ %A


; extended area for dividend is %5 %6 (which will eventually be the remainder)
; area for quotient is %7 %8



BL1_ %5 $00
BL1_ %6 $00
BL1_ %7 $00
BL1_ %8 $00



{ ; do this 32 times

{ ; this shifts the dividend over
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
}





{ ; this attempts subtraction into temporary space
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
; subtraction done, carry is in %D
}

{ ; insert appropiate result into dividend
ADDN %9 %C %D
; now, %9 contains FFFF if %D==0 and 0000 if %D==1
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
}


{ ; this shifts the quotient over and inserts carry bit
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %8 %8 %8
OR__ %8 %8 %D
}



}


; signed convertions would be here


; pop the sign xor
POP1 %A

BL1_ %B $01
AND_ %B %B %A
; do sign convertion (remember to change %7->%5 and %8->%6 for modulus)
XOR_ %7 %7 %A
XOR_ %8 %8 %A
ADDC %7 %7 %B
ADDN %8 %8 %F



; push quotient
PU2_ %8 %7



}





; The following are the fully expanded versions of the above with no comments (except to indicate what is what)
; the other files (like 'true div_u_u.asm') contain slightly modified versions 
; (because the ones here were intended to be inline assembly and the ones in the other files are intended to be functions for use with the instructions like D32U)
; Also, another modification was made for the other files. A register renaming happened due to %1 being partitioned as a frame pointer.
;   The rename is as follows, in this order:
;   %9->%E
;   %8->%9
;   %7->%8
;   %6->%7
;   %5->%6
;   %4->%5
;   %3->%4
;   %2->%3
;   %1->%2



; div_u_u


POP2 %1 %2
POP2 %3 %4
BL1_ %5 $00
BL1_ %6 $00
BL1_ %7 $00
BL1_ %8 $00
RL1_ %C $FFFF
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %8 %8 %8
OR__ %8 %8 %D
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %8 %8 %8
OR__ %8 %8 %D
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %8 %8 %8
OR__ %8 %8 %D
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %8 %8 %8
OR__ %8 %8 %D
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %8 %8 %8
OR__ %8 %8 %D
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %8 %8 %8
OR__ %8 %8 %D
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %8 %8 %8
OR__ %8 %8 %D
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %8 %8 %8
OR__ %8 %8 %D
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %8 %8 %8
OR__ %8 %8 %D
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %8 %8 %8
OR__ %8 %8 %D
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %8 %8 %8
OR__ %8 %8 %D
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %8 %8 %8
OR__ %8 %8 %D
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %8 %8 %8
OR__ %8 %8 %D
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %8 %8 %8
OR__ %8 %8 %D
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %8 %8 %8
OR__ %8 %8 %D
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %8 %8 %8
OR__ %8 %8 %D
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %8 %8 %8
OR__ %8 %8 %D
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %8 %8 %8
OR__ %8 %8 %D
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %8 %8 %8
OR__ %8 %8 %D
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %8 %8 %8
OR__ %8 %8 %D
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %8 %8 %8
OR__ %8 %8 %D
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %8 %8 %8
OR__ %8 %8 %D
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %8 %8 %8
OR__ %8 %8 %D
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %8 %8 %8
OR__ %8 %8 %D
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %8 %8 %8
OR__ %8 %8 %D
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %8 %8 %8
OR__ %8 %8 %D
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %8 %8 %8
OR__ %8 %8 %D
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %8 %8 %8
OR__ %8 %8 %D
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %8 %8 %8
OR__ %8 %8 %D
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %8 %8 %8
OR__ %8 %8 %D
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %8 %8 %8
OR__ %8 %8 %D
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %8 %8 %8
OR__ %8 %8 %D
PU2_ %8 %7





; mod_u_u


POP2 %1 %2
POP2 %3 %4
BL1_ %5 $00
BL1_ %6 $00
BL1_ %7 $00
BL1_ %8 $00
RL1_ %C $FFFF
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
PU2_ %6 %5








; div_s_s




POP2 %1 %2
POP2 %3 %4
RL1_ %C $FFFF
BL1_ %D $80
BSWP %D %D
AND_ %5 %2 %D
ADDC %5 %5 %D
ADDN %5 %F %C
XOR_ %5 %5 %C
MOV_ %A %5
MOV_ %6 %F
XOR_ %1 %1 %5
XOR_ %2 %2 %5
ADDC %1 %1 %6
ADDN %2 %2 %F
AND_ %5 %4 %D
ADDC %5 %5 %D
ADDN %5 %F %C
XOR_ %5 %5 %C
XOR_ %A %A %5
MOV_ %6 %F
XOR_ %3 %3 %5
XOR_ %4 %4 %5
ADDC %3 %3 %6
ADDN %4 %4 %F
PU1_ %A
BL1_ %5 $00
BL1_ %6 $00
BL1_ %7 $00
BL1_ %8 $00
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %8 %8 %8
OR__ %8 %8 %D
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %8 %8 %8
OR__ %8 %8 %D
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %8 %8 %8
OR__ %8 %8 %D
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %8 %8 %8
OR__ %8 %8 %D
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %8 %8 %8
OR__ %8 %8 %D
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %8 %8 %8
OR__ %8 %8 %D
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %8 %8 %8
OR__ %8 %8 %D
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %8 %8 %8
OR__ %8 %8 %D
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %8 %8 %8
OR__ %8 %8 %D
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %8 %8 %8
OR__ %8 %8 %D
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %8 %8 %8
OR__ %8 %8 %D
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %8 %8 %8
OR__ %8 %8 %D
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %8 %8 %8
OR__ %8 %8 %D
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %8 %8 %8
OR__ %8 %8 %D
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %8 %8 %8
OR__ %8 %8 %D
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %8 %8 %8
OR__ %8 %8 %D
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %8 %8 %8
OR__ %8 %8 %D
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %8 %8 %8
OR__ %8 %8 %D
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %8 %8 %8
OR__ %8 %8 %D
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %8 %8 %8
OR__ %8 %8 %D
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %8 %8 %8
OR__ %8 %8 %D
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %8 %8 %8
OR__ %8 %8 %D
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %8 %8 %8
OR__ %8 %8 %D
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %8 %8 %8
OR__ %8 %8 %D
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %8 %8 %8
OR__ %8 %8 %D
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %8 %8 %8
OR__ %8 %8 %D
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %8 %8 %8
OR__ %8 %8 %D
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %8 %8 %8
OR__ %8 %8 %D
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %8 %8 %8
OR__ %8 %8 %D
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %8 %8 %8
OR__ %8 %8 %D
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %8 %8 %8
OR__ %8 %8 %D
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %7 %7 %7
OR__ %7 %7 %D
MOV_ %D %F
ADDN %8 %8 %8
OR__ %8 %8 %D
POP1 %A
BL1_ %B $01
AND_ %B %B %A
XOR_ %7 %7 %A
XOR_ %8 %8 %A
ADDC %7 %7 %B
ADDN %8 %8 %F
PU2_ %8 %7








; mod_s_s




POP2 %1 %2
POP2 %3 %4
RL1_ %C $FFFF
BL1_ %D $80
BSWP %D %D
AND_ %5 %2 %D
ADDC %5 %5 %D
ADDN %5 %F %C
XOR_ %5 %5 %C
MOV_ %A %5
MOV_ %6 %F
XOR_ %1 %1 %5
XOR_ %2 %2 %5
ADDC %1 %1 %6
ADDN %2 %2 %F
AND_ %5 %4 %D
ADDC %5 %5 %D
ADDN %5 %F %C
XOR_ %5 %5 %C
MOV_ %6 %F
XOR_ %3 %3 %5
XOR_ %4 %4 %5
ADDC %3 %3 %6
ADDN %4 %4 %F
PU1_ %A
BL1_ %5 $00
BL1_ %6 $00
BL1_ %7 $00
BL1_ %8 $00
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
ADDC %3 %3 %3
MOV_ %D %F
ADDC %4 %4 %4
OR__ %4 %4 %D
MOV_ %D %F
ADDC %5 %5 %5
OR__ %5 %5 %D
MOV_ %D %F
ADDN %6 %6 %6
OR__ %6 %6 %D
BL1_ %D $01
MOV_ %A %5
MOV_ %B %6
SSUB %D %A %1
SSUB %D %B %2
ADDN %9 %C %D
AND_ %5 %5 %9
AND_ %6 %6 %9
XOR_ %9 %9 %C
AND_ %A %A %9
AND_ %B %B %9
OR__ %5 %5 %A
OR__ %6 %6 %B
POP1 %A
BL1_ %B $01
AND_ %B %B %A
XOR_ %5 %5 %A
XOR_ %6 %6 %A
ADDC %5 %5 %B
ADDN %6 %6 %F
PU2_ %6 %5









