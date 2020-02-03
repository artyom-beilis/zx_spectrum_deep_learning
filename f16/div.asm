GLOBAL _div_24_by_15
GLOBAL _div_24_by_15_ehl_by_bc

SECTION code_compiler

;
; D=de
; R=hl
; N=chl'
; Q=bde'
; 


_div_24_by_15:
    push ix
    ld ix,0
    add ix,sp
    ld l,(ix+4)
    ld h,(ix+5)
    ld e,(ix+6)

    ld c,(ix+8)
    ld b,(ix+9)
    pop ix

_div_24_by_15_ehl_by_bc:
    push hl
    ld a,e
    exx 
    ld c,a
    ex (sp),hl  ; chl = N ; save hl' on stack
    ld de,0
    ld b,d      ; bde = Q = 0
    exx
    ld hl,0
    ld e,c
    ld d,b
    ld b,24
div_int_next:
    
    exx 
    sla e ; Q <<= 1
    rl d
    rl b
    
    sla l  ; N<<=1
    rl h 
    rl c
    exx
    
    adc hl,hl ; R=R*2 + msb(N)
    sbc hl,de ; since hl <= 65536 don't need to clear carry
    jr nc,div_int_update_after_substr
    add hl,de ; fix sub
    djnz div_int_next
    jr div_int_done
div_int_update_after_substr:
    exx    ; Q++
    inc e  
    exx
    djnz div_int_next
div_int_done:
    ex (sp),hl ; restore hl' (speccy thing) and put reminder to the stack
    exx
    ex de,hl
    ld e,b
    ld d,0
    pop bc ; get  reminder
    ret 
