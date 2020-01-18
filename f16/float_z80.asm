GLOBAL _fadd_hl_de
GLOBAL _fsub_hl_de
GLOBAL _f16_add
GLOBAL _f16_sub
; calc fp16 A-B 
; input A hl, B de
; result A-B hl


SECTION code_compiler

_f16_add:
    push ix
    ld ix,0
    add ix,sp
	ld	l,(ix+4)
	ld	h,(ix+5)
	ld	e,(ix+6)
	ld	d,(ix+7)
    call _fadd_hl_de
    pop ix
    ret

_f16_sub:
    push ix
    ld ix,0
    add ix,sp
	ld	l,(ix+4)
	ld	h,(ix+5)
	ld	e,(ix+6)
	ld	d,(ix+7)
    call _fsub_hl_de
    pop ix
    ret



_fsub_hl_de:
    ld a,d      ; if B==0 return A
    and 0x7f
    or e
    ret z
    ld a,d      ; B=-B
    xor 0x80    
    ld d,a      ; continue to A+B
_fadd_hl_de:
    ld a,h
    and 0x80
    ld (sign),a  ; store sign(A)
    xor d
    and 0x80
    ld (op_sub),a   ; op_sub = sign(A) ^ sign(B)
    ;  handle zeros
    ld a,h          ; if A==0 return B
    and 0x7F
    or l
    jr nz,hl_not_zero
    ex de,hl
    ret
hl_not_zero:        ; if B == 0 return A
    ld a,d
    and 0x7f
    or e
    ret z
    ; handle invalids
    ld c,0x7C       ; if exp(A) == 31 goto return nan
    ld a,h
    and c
    cp c
    jp z,return_nan
    ld a,d          ; if exp(B) == 31 goto return nan
    and c
    cp c
    jp z,return_nan

    res 7,h         ; we don't keep sing any more
    res 7,d         ; A=abs(A) , B=abs(B)

    ld a,l          ; check if A-B>=0 no swap
    sub e
    ld a,h
    sbc d
    jr nc,no_swap
    ex de,hl        ; swap(A,B)
    ld a,(op_sub)   ; if op_sub -> sign = -sign
    ld c,a
    ld a,(sign)
    xor c
    ld (sign),a
no_swap:
    ld a,d
    and 0x7C
    ld c,a     ; c=exp(A)<<2
    ld a,3      ; B = B & 1023
    and d
    ld d,a
    set 2,d     ; B = B | 1024
    ld a,c
    rra
    rra         ; a=exp(A)
    and a               ; if EXPONENT(B) == 0 a=1
    jr nz,dont_inc_ax
    res 2,d             ; reset 1024 bit for subnormal
    inc a               ; de = mantissa(B),
dont_inc_ax:
    ld b,a              ; de = mantissa(B), b=exponent + (1 if B subnormal)
    ld a,h
    and 0x7C
    ld c,a
    ld a,3
    and h
    ld h,a
    set 2,h
    ld a,c
    rra
    rra
    ld (exp),a          ; exp = EXP(A)
    and a
    jr nz,dont_inc_bx
    inc a
    res 2,h
dont_inc_bx:
    sub b               ; b=ax - bx
    ld b,a
    ld a,(op_sub)       ; if op_sub - go to substr
    and a
    jr nz,substruction
    ld a,b              ; if ax - bx == 0 no shoft
    and a
    jr z,no_diff_add    ; 
shift_de_add:
    sra d
    rr e
    djnz shift_de_add    ; de>> b
no_diff_add:
    add hl,de           ; hl = m1+m2
    ld a,(exp)          ; 
    bit 3,h             ; hl & 2048 exp++, hl >>=1
    jr z,final_combine
    inc a
    sra h
    rr l
    jr final_combine
substruction:
    add hl,hl   ; m1<<=1, m2<<=1
    ex de,hl
    add hl,hl
    ex de,hl
    ld a,b
    and a
    jr z,no_diff_sub
shift_de_sub:   ; m2 >>= diff
    sra d
    rr e
    djnz shift_de_sub
no_diff_sub:
    ld a,l
    sub e
    ld l,a
    ld a,h
    sbc d   ; m1 = m1-m2
    ld h,a
    or l
    ret z ; if diff = 0 return 0
    ld a,(exp)              ; while exp > 0 and bit 2048 not set
next_shift_sub:
    bit 3,h
    jr nz,sub_shift_done
    and a,a
    jr z,sub_shift_done
    add hl,hl                   ; m1<<1, exp--
    dec a
    jr next_shift_sub       ; end while
sub_shift_done:
    and a                 ;if exp == 0 m1 >>= 2 else m1 >>= 1
    jr nz,shift_once
    sra h
    rr l
shift_once:
    sra h
    rr l
final_combine:              
    cp 31
    jr nc, return_inf
    add a
    add a 
    ld b,a
    ld a,(sign)
    or b
    res 2,h ; remove hidden bit
    or h
    ld h,a
    ret
  
return_inf:
    ld a,(sign)
    or 0x7C
    ld h,a
    ld l,0
    ret
    
return_nan:
    ld hl,0x7FFF
    ret
     
op_sub: defb 0
sign  : defb 0
exp   : defb 0

