GLOBAL _get_addr_and_mask
GLOBAL _get_addr_and_mask_e_l
GLOBAL _inc_y_addr_and_mask
GLOBAL _inc_x_addr_and_mask
GLOBAL _dec_x_addr_and_mask
GLOBAL _draw_line
GLOBAL _plot
GLOBAL _clear_screen
GLOBAL _show_screen
GLOBAL _screen_buffer_addr
;   inp: e=y, l=x
;   out  hl addr e,mask

SECTION code_compiler


_screen_buffer_addr: defw 0x6800
_screen_size: defw 4096
;_screen_buffer_addr: defw 0x4000

_show_screen:
    ld de,16384
    ld hl,(_screen_buffer_addr)
    ld bc,(_screen_size)
    ldir
    ret

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
    ld a,(_screen_buffer_addr+1)
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
    ld hl,0
    add hl,sp
    ex de,hl
    ld hl,(_screen_size)
    ld sp,(_screen_buffer_addr)
    add hl,sp
    ld sp,hl
    ex de,hl
    ld de,0
    ld b,0
cls_next:
    ;push de
    ;push de
    ;push de
    ;push de

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

    ld d,(ix+x0)        ;;;;;;
    ld e,(ix+y0)
    ld h,(ix+x1)
    ld l,(ix+y1)

    ld a,l
    cp e
    jp z,horizontal_line
    jr nc,no_swap_x0y0_x1y1
    ; swap x0<->x1, y0<->y1

    ex de,hl
    ; save original values for starting point
    ld (ix+x0),d
    ld (ix+y0),e

no_swap_x0y0_x1y1:
    
    ld bc,_inc_y_addr_and_mask
    ld (inc_y_addr),bc
    ld bc,_inc_x_addr_and_mask
    ld (inc_x_addr),bc

    ld a,h
    sub d
    jp z,vertical_line
    jr nc, x1_x0_order_ok
    ld bc,_dec_x_addr_and_mask
    ld (inc_x_addr),bc
    neg
    ; swap x0,x1 - swap both and restore
    ex de,hl
    ld c,l
    ld l,e
    ld e,c
x1_x0_order_ok:
    ld c,a ; save abs(dx)
    ld a,l
    sub e
    cp c
    jr c,dy_below_dx

    ; swap x0<->y0    
    ld c,d
    ld d,e
    ld e,c
   
    ; swap x1<->y1
    ld c,h
    ld h,l
    ld l,c 
   
    ld bc,(inc_x_addr)
    ld (inc_y_addr),bc
    ld bc,_inc_y_addr_and_mask
    ld (inc_x_addr),bc

dy_below_dx:

    ld a,h
    sub d
    ld c,a          ; save dx to c as well
    ld (ix+dx),a   ; dx = x1 - x0
    inc a
    ld (ix+x_steps),a ; x_steps = dx+1
    ld a,l 
    sub e
    ld (ix+dy),a    ; dy = y1 - y0
    ld a,c   ; a=dx saved n c above
    or a ; reset C
    rr a                 ; a=dx/2
    neg a   
    add a,(ix+dy)       ; a=-dx/2+dy (low)
    ld c,a
    ld a,255
    adc 0
    ld b,a;             ; bc=(-dx/2+dy)
    ld l,(ix+x0)  ; original
    ld e,(ix+y0)  ; original
    push bc
    call _get_addr_and_mask_e_l  ; calc address
    pop bc
    ld d,(ix+dy)    ; d=dy
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
inc_y_addr: defw 0
    ;call virtual_inc_y_subr
    ld a,c          ; D(bc)-= dx
    sub (ix+dx)
    ld c,a
    ld a,b
    sbc 0
    ld b,a

D_is_not_pos:
    ld a,c          ; D(bc) +=dy
    add d
    ld c,a
    ld a,b
    adc 0
    ld b,a
    
    ;call virtual_inc_x_subr
    defb 0xCD
inc_x_addr: defw 0
    
    dec (ix+x_steps)
    jr nz,while_next

done:
    pop af
    pop af
    pop ix
    ret

horizontal_line:
    ld a,h
    sub d
    jr nc,dont_change_hline_order
    ex de,hl
    neg
dont_change_hline_order:
    inc a
    ld (ix+x_steps),a
    ld l,d ; l=x0
    call _get_addr_and_mask_e_l
    ld b,(ix+x_steps)
    ld c,8
next_hpoint:
    ld a,0x80
    xor e
    jr nz,pixel_pixel
    ld d,0xFF
next_h8:
    ld a,c
    cp b
    jr nc,pixel_pixel
    ld (hl),d
    inc hl
    ld a,b
    sub c
    ld b,a
    jr next_h8

pixel_pixel:
    ld a,(hl)
    or e
    ld (hl),a
    call _inc_x_addr_and_mask
    djnz next_hpoint
    jr done

vertical_line:
    ld a,l
    sub e
    inc a
    ld (ix+x_steps),a
    ld l,d ; l=x0
    call _get_addr_and_mask_e_l
    ld b,(ix+x_steps)
next_vpoint:
    ld a,(hl)
    or e
    ld (hl),a
    call _inc_y_addr_and_mask
    djnz next_vpoint
    jr done
