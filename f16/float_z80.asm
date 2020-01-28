GLOBAL _fadd_hl_de
GLOBAL _fsub_hl_de
GLOBAL _fmul_hl_de
GLOBAL _f16_add
GLOBAL _f16_sub
GLOBAL _f16_mul
GLOBAL _mpl_11_bits

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
    ld b,0x80 ; sign mask
    and b
    ld (ix + sign),a  ; store sign(A)
    ld c,a ; save sign to c
    xor d
    and b
    ld (ix + op_sub),a   ; op_sub = sign(A) ^ sign(B)
    ld b,a ; save sub to b

    res 7,h         ; we don't keep sing any more
    res 7,d         ; A=abs(A) , B=abs(B)

    ld a,l          ; check if A-B>=0 no swap
    sub e
    ld a,h
    sbc d
    jr nc,no_swap
    ex de,hl        ; swap(A,B)
    ld a,b   ; if op_sub -> sign = -sign
    xor c
    ld (ix+sign),a
no_swap:
    call calc_ax_bx_mantissa_on_abs
    ld (ix+exp),c          ; exp = EXP(A)
    sub b                  ; b=ax - bx
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

_f16_mul:
    push ix
    ld ix,0
    add ix,sp
 	ld	l,(ix+4)
	ld	h,(ix+5)
	ld	e,(ix+6)
	ld	d,(ix+7)
    jr fmul_entry_after_stack_prepare
     
_fmul_hl_de:
    push ix
    ld ix,0
    add ix,sp

fmul_entry_after_stack_prepare:
    push af
    push af
    call check_invalid
    jp z,return_nan
    ld a,h
    xor d
    and 0x80
    ld (ix + sign),a  ; sign = sig(A) ^ sig(B)
    call calc_ax_bx_mantissa
    add b   ; new_exp = ax + bx - 15
    sub 15
    ld (ix+exp),a 
    ld a,h
    or l
    jp z,exit_point
    ex de,hl
    ld a,h
    or l
    jp z,exit_point
    call mpl_11_bit  ; de/hl = m1*m2
    ld a,(ix+exp)
    bit 5,e                      ; if v >=2048: m>>11, exp++
    jr nz,fmul_exp_inc_shift_11
    bit 4,e
    jr nz,fmul_shift_10         ; if v>=1024: m>>10
    jr fmul_handle_denormals    ; check denormals
fmul_exp_inc_shift_11:
    inc a
    sra e
    rr h
    rr l
fmul_shift_10:
    sra e
    rr h
    rr l
    sra e
    rr h
    rr l
    ld l,h ; ehl >> 8
    ld h,e
    ld e,0
    jr fmul_check_exponent
fmul_handle_denormals:
    sub a,10
    ld c,a
    ld b,0xf8
fmul_next_shift:    ; while ehl >= 2048
    ld a,h
    and b
    or e
    jr z,fmul_shift_loop_end
    sra e           ; ehl >> = 1, exp++
    rr h
    rr l
    inc c
    jr fmul_next_shift
fmul_shift_loop_end:
    ld a,c      ; restre exp
fmul_check_exponent:
    ld b,1
    and a
    jr z,fmul_denorm_shift
    bit 7,a
    jp z,final_combine
    neg 
    add a,b
    ld b,a
    xor a
fmul_denorm_shift:
    sra e
    rr h
    rr l
    djnz fmul_denorm_shift
    jp final_combine
    

    ; input hl,de
    ; output 
    ;   a=ax = max(exp(hl),1)
    ;   b=bx = max(exp(de),1)
    ;   c = exp(hl)
    ;   hl = mantissa(hl)
    ;   de = mantissa(de)


calc_ax_bx_mantissa:
    res 7,h
    res 7,d
calc_ax_bx_mantissa_on_abs:
    ld a,d
    and 0x7c
    jr z,bx_is_zero
    rra  
    rra 
    ld b,a ; b=bx
    ld a,3
    and d      ; keep bits 0,1, set 2
    or 4      
    ld d,a     ; de = mantissa
    jr bx_not_zero 
bx_is_zero:
    ld b,1 ; de is already ok since exp=0, bit 7 reset
bx_not_zero:
    ; b is bx, de=mantissa(de)
    ld a,h
    and 0x7c
    jr z,ax_is_zero 
    rra
    rra 
    ld c,a  ; c=exp
    ld a,3
    and h
    or 4
    ld h,a
    ld a,c ; exp=ax
    ret
ax_is_zero:
    ld a,1 ; hl is already ok a=ax
    ld c,0 ; c=exp(hl)
    ret 


_mpl_11_bits:
    push ix
    ld ix,0
    add ix,sp
 	ld	l,(ix+4)
	ld	h,(ix+5)
	ld	e,(ix+6)
    ld  d,(ix+7)
    call mpl_11_bit
    pop ix
    ret

mpl_11_bit:
    ld b,d
    ld c,e
    ld d,h
    ld e,l
    xor a,a
    bit 2,b
	jr	nz,unroll_a
	ld	h,a  
	ld	l,a
unroll_a:
    add hl,hl
    rla
    bit 1,b
    jr  z,unroll_b
	add hl,de
    adc 0
unroll_b:
    add hl,hl
    rla
    bit 0,b
    jr z,unroll_c
    add hl,de
    adc 0
unroll_c:

    ld b,8

mpl_loop2:

	add	hl,hl  ; 
    rla
	rl c	   ; carry goes to de low bit
	jr	nc,mpl_end2
	add	hl,de
	adc 0
mpl_end2:

    djnz mpl_loop2
    ld e,a
    ld d,0
    ret

