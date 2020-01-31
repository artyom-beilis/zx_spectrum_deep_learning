GLOBAL _f16_add_hl_de
GLOBAL _f16_sub_hl_de
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

_f16_add:
    pop bc
    pop hl
    pop de
    push de
    push hl
    push bc
_f16_add_hl_de:
    ld a,h
    xor d
    and 0x80
    jr z,fast_add_do_add
    xor d
    ld d,a
    jp _f16_sub_hl_de
fast_add_do_add:
    ld a,0x80
    and h
    ex af,af' ; save sign in AF'
    res 7,d
    res 7,h
    ld a,l
    sub e
    ld a,h
    sbc d
    jr nc,fast_add_noswap
    ex de,hl
fast_add_noswap:
    ld c,0x7C
    ld a,c
    and d
    cp c
    jp z,handle_nan
    ld b,a ; bx in b
    ld a,h
    and c
    cp c
    jp z,handle_nan
    sub b
    jr z,fast_add_diff_0
    rra  ; carry is reset due to 
    rra  ; low bits are zero so no problem
    ld c,a ; c=shift
    ld a,b ; 
    or a 
    jr z, fast_add_bx_is_zero
    ld a,d
    and 3
    or 4
    ld d,a
    ld b,c
fast_add_shift_a:
    sra d
    rr e
    djnz fast_add_shift_a
    jr fast_add_actual_add
fast_add_bx_is_zero:
    dec c
    jr z,fast_add_actual_add
    ld b,c
fast_add_shift_b:
    sra d
    rr e
    djnz fast_add_shift_b
    jr fast_add_actual_add
fast_add_diff_0:
    ld a,b
    or a
    jr nz,fast_add_bx_not_zero_diff_zero
    add hl,de
    ex af,af'
    or a,h
    ld h,a
    ret
fast_add_bx_not_zero_diff_zero:
    ld a,d
    and 3
    or 4
    ld d,a
fast_add_actual_add: 
    ld b,h  ; save hl
    ld c,l
    add hl,de  ; compare (hl+de) & 0x7c00 == 0x7c00 & hl stored in bc
    ld a,b
    xor h
    and 0x7C
    jr nz,fast_add_update_exp 
    ex af,af'
    or a,h
    ld h,a
    ret
fast_add_update_exp:
    ld a,b
    and 3
    or 4
    ld h,a
    ld l,c
    add hl,de
    sra h
    rr l
    res 2,h
    ld a,b
    and 0x7C
    add 4
    ld b,a
    ex af,af'
    or b
    or h
    ld h,a
    ret


_f16_sub:
    pop bc
    pop hl
    pop de
    push de
    push hl
    push bc
_f16_sub_hl_de:
    ld a,h
    xor d
    and 0x80
    jr z,fast_sub_do_sub
    xor d
    ld d,a
    jp _f16_add_hl_de
fast_sub_do_sub:
    ld a,0x80
    and h
    ex af,af' ; save sign in AF'
    res 7,d
    and a
    rl e
    rl d
    res 7,h
    and a
    rl l
    rl h  ; shift both by 1 bit
    ld a,l
    sub e
    ld a,h
    sbc d
    jr nc,fast_sub_noswap
    ex de,hl
    ex af,af'
    xor 0x80
    ex af,af'
fast_sub_noswap:
    ld c,0xF8
    ld a,c
    and d
    cp c
    jp z,handle_nan
    ld b,a ; bx in b
    ld a,h
    and c
    cp c
    jr z,handle_nan
    sub b
    jr z,fast_sub_diff_0

    rra  ; carry is reset due to 
    rra  ; low bits are zero so no problem
    rra 

    ld c,a ; c=shift
    ld a,b ; 
    or a 
    jr z, fast_sub_bx_is_zero
    ld a,d
    and 7
    or 8
    ld d,a
    ld b,c
fast_sub_shift_a:
    sra d
    rr e
    djnz fast_sub_shift_a
    jr fast_sub_actual_sub
fast_sub_bx_is_zero:
    dec c
    jr z,fast_sub_actual_sub
    ld b,c
fast_sub_shift_b:
    sra d
    rr e
    djnz fast_sub_shift_b
    jr fast_sub_actual_sub
fast_sub_diff_0:        
    ld a,b
    or a
    jr nz,fast_sub_bx_not_zero_diff_zero
    sbc hl,de
    jr fast_sub_shift_add_sign_and_ret
fast_sub_bx_not_zero_diff_zero:
    ld a,d
    and 7
    or 8
    ld d,a
fast_sub_actual_sub: 
    ld b,h  ; save hl
    ld c,l
    and a ; reset carry
    sbc hl,de
    ; compare (hl-de) & 0xf800 == 0xf800 & hl stored in bc
    ld a,b
    xor h
    and 0xf8
    jr z,fast_sub_shift_add_sign_and_ret
    
    ld a,b
    and 7
    or 8
    ld h,a
    ld l,c
    sbc hl,de
    jr z,fast_sub_shift_add_sign_and_ret
    ld a,0xF8
    and b
    jr z,fast_sub_break_shift_loop
fast_sub_shift_loop:
    bit 3,h
    jr nz,fast_sub_break_shift_loop
    sub a,8
    jr z,fast_sub_break_shift_loop
    add hl,hl
    jr fast_sub_shift_loop
fast_sub_break_shift_loop:
    res 3,h
    or h
    ld h,a
fast_sub_shift_add_sign_and_ret:
    and a ; reset C
    rr h
    rr l 
    ld a,h ; check for 0 - make sure not to return -0
    or l
    ret z
    ex af,af'
    or a,h
    ld h,a
    ret

handle_nan:
    ld hl,0x7FFF
    ret




_f16_mul:
    pop bc
    pop hl
    pop de
    push de
    push hl
    push bc
_fmul_hl_de:
    ld a,h
    xor d
    and 0x80
    ex af,af'
    call calc_ax_bx_mantissa
    jr z,handle_nan
    add b   ; new_exp = ax + bx - 15
    sub 15
    exx 
    ld c,a ; save exp in bc'
    exx
    ld a,h
    or l
    ret z
    ex de,hl
    ld a,h
    or l
    ret z
    call mpl_11_bit  ; de/hl = m1*m2
    exx
    ld a,c
    exx 
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
final_combine:              
    cp 31
    jr nc, handle_nan
    add a
    add a 
    res 2,h ; remove hidden bit
    or h
    ld h,a
    or 0x7F         ; reset -0 to normal 0
    or l
    ret z
    ex af,af' ; restore sign
    or h
    ld h,a
    ret
    

    ; input hl,de
    ; output 
    ;   a=ax = max(exp(hl),1)
    ;   b=bx = max(exp(de),1)
    ;   c = exp(hl)
    ;   hl = mantissa(hl)
    ;   de = mantissa(de)
    ;   z flag one of the numbers is inf/nan
    ;   no calcs done


calc_ax_bx_mantissa:
    res 7,h
    res 7,d
    ld c,0x7c
    ld a,d
    and c
    jr z,bx_is_zero
    cp c
    ret z
    rrca  
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
    and c
    jr z,ax_is_zero 
    cp c
    ret z
    rrca
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
    and a ; reset z flag
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

