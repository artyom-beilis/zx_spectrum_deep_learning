GLOBAL _fadd_hl_de
GLOBAL _fsub_hl_de
GLOBAL _f16_add
GLOBAL _f16_sub
; calc fp16 A-B 
; input A hl, B de
; result A-B hl


op_sub  equ -2
sign    equ -3
exp     equ -4



SECTION code_compiler


_f16_sub:
    push ix
    ld ix,0
    add ix,sp
 	ld	l,(ix+4)
	ld	h,(ix+5)
	ld	e,(ix+6)
    ld a,0x80
	xor (ix+7)
    ld d,a
    jr fadd_entry_after_stack_prepare

_f16_add:
    push ix
    ld ix,0
    add ix,sp
 	ld	l,(ix+4)
	ld	h,(ix+5)
	ld	e,(ix+6)
	ld	d,(ix+7)
    jr fadd_entry_after_stack_prepare

_fsub_hl_de:
    and a,0x80
    xor d
    ld d,a

_fadd_hl_de:
    push ix
    ld ix,0
    add ix,sp
fadd_entry_after_stack_prepare:
    push af
    push af
    call check_invalid
    jp z,return_nan
    ld a,h
    and 0x80
    ld (ix + sign),a  ; store sign(A)
    xor d
    and 0x80
    ld (ix + op_sub),a   ; op_sub = sign(A) ^ sign(B)

    res 7,h         ; we don't keep sing any more
    res 7,d         ; A=abs(A) , B=abs(B)

    ld a,l          ; check if A-B>=0 no swap
    sub e
    ld a,h
    sbc d
    jr nc,no_swap
    ex de,hl        ; swap(A,B)
    ld a,(ix+op_sub)   ; if op_sub -> sign = -sign
    xor (ix+sign)
    ld (ix+sign),a
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
    ld (ix+exp),a          ; exp = EXP(A)
    and a
    jr nz,dont_inc_bx
    inc a
    res 2,h
dont_inc_bx:
    sub b               ; b=ax - bx
    ld b,a
    ld a,(ix+op_sub)       ; if op_sub - go to substr
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
    ld a,(ix+exp)          ; 
    bit 3,h             ; hl & 2048 exp++, hl >>=1
    jr z,fix_subnormal_add_exp
    inc a
    sra h
    rr l
    jr final_combine
fix_subnormal_add_exp:  ; if exp==0 and hl & 1024 exp++
    and a
    jr nz,final_combine
    bit 2,h
    jr z,final_combine
    inc a
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
    jp z,exit_point ; if diff = 0 return 0
    ld a,(ix+exp)              ; while exp > 0 and bit 2048 not set
next_shift_sub:
    bit 3,h
    jr nz,sub_shift_done
    and a,a
    jr z,sub_shift_done
    dec a                       ; exp --
    jr z,sub_shift_done         ; if exp==0 break
    add hl,hl                   ; m1<<=1
    jr next_shift_sub       ; end while
sub_shift_done:
    sra h   ; m1>>=1
    rr l
final_combine:              
    cp 31
    jr nc, return_inf
    add a
    add a 
    or a,(ix+sign)
    res 2,h ; remove hidden bit
    or h
    ld h,a
    or 0x7F         ; reset -0 to normal 0
    or l
    jr nz,exit_point
    ld hl,0
exit_point:
    pop af
    pop af
    pop ix
    ret
  
return_inf:
    ld a,(ix+sign)
    or 0x7C
    ld h,a
    ld l,0
    jr exit_point
return_nan:
    ld hl,0x7FFF
    jr exit_point

check_invalid:
    ; check if hl or de is inf/nan, returns flag z if is invalid
    ld c,0x7C       ; if exp(A) == 31 goto return nan
    ld a,h
    and c
    cp c
    ret z
    ld a,d          ; if exp(B) == 31 goto return nan
    and c
    cp c
    ret

     
;op_sub: defb 0
;sign  : defb 0
;exp   : defb 0

