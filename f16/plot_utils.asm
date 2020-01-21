GLOBAL _get_addr_and_mask
GLOBAL _get_addr_and_mask_e_l
GLOBAL _inc_y_addr_and_mask
GLOBAL _inc_x_addr_and_mask
GLOBAL _dec_x_addr_and_mask
GLOBAL _draw_v1
;   inp: e=y, l=x
;   out  hl addr e,mask

SECTION code_compiler

_get_addr_and_mask:
    push ix
    ld ix,0
    add ix,sp
    ld e,(ix+4)
    ld l,(ix+6)
    call _get_addr_and_mask_e_l
    pop ix
    ret

_inc_x_addr_and_mask:
    ld a,e
    rlca
    ld e,a
    ret nc
    inc l
    ret

_dec_x_addr_and_mask:
    ld a,e
    rrca
    ld e,a
    ret nc
    dec l
    ret

_inc_y_addr_and_mask:
    inc h
    ld a,7
    or h
    ret nz
    ld a,-8
    add h
    ld h,a
    ld a,0x20
    add l
    ret nc
    ld a,8
    add h
    ld h,a
    ret

_get_addr_and_mask_e_l:
    ld h,0  
    ld a,e  ; 2 msb of y to low msb of h
    add a,a
    rl h
    add a,a
    rl h
    or a
    rl h
    rl h
    rl h
    ld a,0x7 ; get 3 lsb of y to a
    and e
    or h
    ld h,a    ; now h contains of all valid address 2 msb of y and 3 lsb of y

    ld a,7
    and l
    ld b,a   ; b is now (3 lsb a)
    
    rr e
    rr e
    rr e
    ld a,l
    and a ; reset carry
    rr e
    rra
    rr e
    rra
    rr e
    rra 
    ld l,a ; l containss 3 middle bits of e at lsb and 5 msbs of ;
    ld a,0x40
    add a,h
    ld h,a  ; hl 16384 + addr
    ld e,128
    ld a,b
    and a
    jr z,no_shift
    ld a,e
next_shift:
    rrca 
    djnz next_shift
    ld e,a
no_shift:
    ld d,0
    ret


x0 equ 4
x1 equ 6
y0 equ 8
y1 equ 10
dx equ -2
dy equ -3
d_err equ -4
x_steps equ -5
_draw_v1:
    push ix
    ld ix,0
    add ix,sp
    push af
    push af
    ld a,(ix+x1)
    sub (ix+x0)
    ld (ix+dx),a
    inc a
    ld (ix+x_steps),a
    ld a,(ix+y1)
    sub (ix+y0)
    ld (ix+dy),a
    add a,a
    sub a,(ix+dx)
    ld (ix+d_err),a
    ld l,(ix+x0)
    ld e,(ix+y0)
    call _get_addr_and_mask_e_l 
    ld b,(ix+x_steps)
    ld c,(ix+d_err)
while_next:
    ld a,e
    or (hl)
    ld (hl),a
    bit 7,c
    jr D_is_not_pos
    ld a,c
    or a
    jr z,D_is_not_pos
    call _inc_y_addr_and_mask
    ld a,c
    sub (ix+dx)
    sub (ix+dx)
    ld c,a
D_is_not_pos:
    ld a,c
    add (ix+dy)
    add (ix+dy)
    ld c,a
    call _inc_x_addr_and_mask
    djnz while_next

    pop af
    pop af
    pop ix
    ret

    




