GLOBAL _get_addr_and_mask
GLOBAL _get_addr_and_mask_e_l
GLOBAL _inc_y_addr_and_mask
GLOBAL _inc_x_addr_and_mask
GLOBAL _dec_x_addr_and_mask
GLOBAL _draw_line
GLOBAL _plot
GLOBAL _clear_screen
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
    rrca
    ld e,a
    ret nc
    inc l
    ret

_dec_x_addr_and_mask:
    ld a,e
    rlca
    ld e,a
    ret nc
    dec l
    ret

_inc_y_addr_and_mask:
    inc h
    ld a,7
    and h
    ret nz
    ld a,-8
    add h
    ld h,a
    ld a,0x20
    add l
    ld l,a
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
x_steps equ -4

_clear_screen:
    di
    ld hl,0
    add hl,sp
    ld de,0
    ld sp,16384+6144
    ld b,0
cls_next:
    push de
    push de
    push de
    push de

    push de
    push de
    push de
    push de

    push de
    push de
    push de
    push de
    djnz cls_next
    ld sp,hl
    ei
    ret

_plot:
    push ix
    ld ix,0
    add ix,sp
    ld a,191
    ld e,(ix+4)
    cp e
    jr c,out_of_range
    ld l,(ix+6)
    call _get_addr_and_mask_e_l
    ld a,e
    or (hl)
    ld (hl),a
out_of_range:
    pop ix
    ret

_draw_line:
    push ix
    ld ix,0
    add ix,sp
    push af
    push af

    ld a,(ix+y1)
    cp (ix+y0)
    jr nc,no_swap_x0y0_x1y1
   
    ld b,(ix+y1)
    ld c,(ix+y0)
    ld (ix+y1),c
    ld (ix+y0),b
    ld b,(ix+x1)
    ld c,(ix+x0)
    ld (ix+x1),c
    ld (ix+x0),b 

no_swap_x0y0_x1y1:
    
    ; save original values for starting point
    ld a,(ix+x0)
    ld (ix+x0+1),a
    ld a,(ix+y0)
    ld (ix+y0+1),a

    ld hl,_inc_y_addr_and_mask
    ld (inc_y_addr),hl
    ld hl,_inc_x_addr_and_mask
    ld (inc_x_addr),hl

    ld a,(ix+x1)
    sub (ix+x0)
    jr nc, x1_x0_order_ok
    ld hl,_dec_x_addr_and_mask
    ld (inc_x_addr),hl
    neg
    ld c,(ix+x1)
    ld b,(ix+x0)
    ld (ix+x1),b
    ld (ix+x0),c
x1_x0_order_ok:
    ld c,a ; save abs(dx)
    ld a,(ix+y1)
    sub (ix+y0)
    cp c
    jr c,dy_below_dx
    ld b,(ix+x0)
    ld c,(ix+y0)
    ld (ix+x0),c
    ld (ix+y0),b
    
    ld b,(ix+x1)
    ld c,(ix+y1)
    ld (ix+x1),c
    ld (ix+y1),b
    
    ld hl,(inc_x_addr)
    ld (inc_y_addr),hl
    ld hl,_inc_y_addr_and_mask
    ld (inc_x_addr),hl
dy_below_dx:

    ld a,(ix+x1)
    sub (ix+x0)
    ld (ix+dx),a   ; dx = x1 - x0
    inc a
    ld (ix+x_steps),a ; x_steps = dx+1
    ld a,(ix+y1)    
    sub (ix+y0)
    ld (ix+dy),a    ; dy = y1 - y0
    ld a,(ix+dx)
    or a ; reset C
    rr a                 ; a=dx/2
    neg a   
    add a,(ix+dy)       ; a=-dx/2+dy (low)
    ld c,a
    ld a,255
    adc 0
    ld b,a;             ; bc=(-dx/2+dy)
    ld l,(ix+x0+1)  ; original
    ld e,(ix+y0+1)  ; original
    push bc
    call _get_addr_and_mask_e_l  ; calc address
    pop bc
while_next:
    ; mem prot
    ;ld a,h
    ;cp 0x40
    ;jr c,done
    ;cp 0x58
    ;jr nc,done

    ld a,e
    or (hl)
    ld (hl),a
    bit 7,b                     ; if D(bc) > 0 (ends at D_is_not_pos)
    jr nz,D_is_not_pos    
    ld a,b
    or c
    jr z,D_is_not_pos
               defb 0xCD
inc_y_addr:    defw 0
    ;call virtual_inc_y_subr
    ld a,c          ; D(bc)-= dx
    sub (ix+dx)
    ld c,a
    ld a,b
    sbc 0
    ld b,a

D_is_not_pos:
    ld a,c          ; D(bc) +=dy
    add (ix+dy)
    ld c,a
    ld a,b
    adc 0
    ld b,a
    
    ;call virtual_inc_x_subr
               defb 0xCD
inc_x_addr:    defw 0
    
    dec (ix+x_steps)
    jr nz,while_next

done:
    pop af
    pop af
    pop ix
    ret

virtual_inc_x_subr:
virtual_inc_y_subr:



