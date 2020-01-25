#ifndef FIXED12_MATH_H
#define FIXED12_MATH_H

int shift_right8(long v) __z88dk_fastcall
{
    __asm
    ld l,h
    ld h,e
    __endasm;
}


int fixed8_mpl(int x,int y)
{
    __asm
	ld	e,(ix+6)
	ld	d,(ix+7)
	ld	c,(ix+4)
	ld	b,(ix+5)

    call mpl_signed_bc_de_to_de_hl

    ; round de:hl 
    ld bc,128
    add hl,bc
    jr nc,mpl_no_de_inc
    inc de

mpl_no_de_inc:
    
    ld l,h
    ld h,e

    __endasm;
}

long mpl_2int_to_long(int x,int y)
{
    __asm

	ld	e,(ix+6)
	ld	d,(ix+7)
	ld	c,(ix+4)
	ld	b,(ix+5)

    call mpl_signed_bc_de_to_de_hl
    jp mpl_done
    
mpl_signed_bc_de_to_de_hl:
    ld a,d
    xor a,b
    push af
    
    bit 7,d
    jr z,mpl_not_neg_de
    
    xor a
    sub e
    ld e,a
    sbc a,a
    sub d
    ld d,a
    
mpl_not_neg_de:
    
    bit 7,b
    jr z,mpl_not_neg_bc
    
    xor a
    sub c
    ld c,a
    sbc a,a
    sub b
    ld b,a

mpl_not_neg_bc:


    ld  hl,0
    sla	e		; optimised 1st iteration
	rl	d
	jr	nc,mpl_loop_start
	ld	h,b
	ld	l,c
mpl_loop_start:
	ld	a,15
mpl_lp:
	add	hl,hl		; unroll 1 times
	rl	e		; ...
	rl	d		; ...
	jr	nc,mpl_end		; ...
	add	hl,bc		; ...
	jr	nc,mpl_end		; ...
	inc	de		; ...
mpl_end:
    dec a
    jr NZ,mpl_lp

    pop af
    bit 7,a
    
    jr z,mpl_no_neg_res
    
    xor a
    ld b,a

    sub l
    ld l,a

    ld a,b
    sbc h
    ld h,a
    
    ld a,b
    sbc e
    ld e,a
    
    ld a,b
    sbc d
    ld d,a

mpl_no_neg_res:
    ret

mpl_done:
    
    __endasm;

}

#endif // FIXED12_MATH_H
