#include "float16.h"
#include <stdint.h>
#ifndef F16_NO_IO
#include <stdio.h>
#endif

#define SIGN_MASK 0x8000
#define EXP_MASK 0x7C00
#define NAN_VALUE 0x7FFF
#define IS_ZERO(x) (((x) & 0x7FFF) == 0)
#define IS_INVALID(x) (((x) & EXP_MASK) == EXP_MASK)
#define IS_INF(x) ( ((x) & 0x7FFF) == 0x7C00)
#define MANTISSA(x) (((x) & 1023) | (((x) & 0x7C00) == 0 ? 0 : 1024))
#define EXPONENT(x) (((x) & 0x7C00) >> 10)
#define SIGNED_INF_VALUE(x)  ((x & SIGN_MASK) | 0x7C00)
/*extern void fadd_hl_de();
extern void fsub_hl_de();
short f16_add(short a,short b)
{
    __asm
        ld l,(ix+4)
        ld h,(ix+5)
        ld e,(ix+6)
        ld d,(ix+7)
        call _fadd_hl_de
    __endasm;
}

short f16_sub(short a,short b)
{
    __asm
        ld l,(ix+4)
        ld h,(ix+5)
        ld e,(ix+6)
        ld d,(ix+7)
        call _fsub_hl_de
    __endasm;
}
*/

#ifndef __linux 
#define f16_add f16_add2
#define f16_sub f16_sub2
#define f16_mul f16_mul2
short f16_add(short a,short b);
short f16_sub(short a,short b);

#endif

short f16_sub(short ain,short bin)
{
    unsigned short a=ain;
    unsigned short b=bin;
    if(((a ^ b) & 0x8000) != 0)
        return f16_add(a,b ^ 0x8000);
    unsigned short sign = a & 0x8000;
    a = a << 1;
    b = b << 1;
    if(a < b) {
        unsigned short x=a;
        a=b;
        b=x;
        sign ^= 0x8000;
    }
    unsigned short ax = a & 0xF800;
    unsigned short bx = b & 0xF800;
    if(ax == 0xf8)
        return 0x7FFF; 
    int exp_diff = ax - bx;
    unsigned short exp_part  = ax;
    if(exp_diff != 0) {
        int shift = exp_diff >> 11;
        if(bx != 0)
            b = ((b & 2047) | 2048) >> shift;
        else
            b >>= (shift - 1);
    }
    else {
        if(bx == 0) {
            unsigned short res = (a-b) >> 1;
            if(res == 0)
                return res;
            return res | sign;
        }
        else {
            b=(b & 2047) | 2048;
        }
    }
    unsigned short r=a - b;
    if((r & 0xF800) == exp_part) {
        return (r>>1) | sign;
    }
    unsigned short am = (a & 2047) | 2048;
    unsigned short new_m = am - b;
    if(new_m == 0)
        return 0;
    while(exp_part !=0 && !(new_m & (2048))) {
        exp_part-=0x800;
        if(exp_part!=0)
            new_m<<=1;
    }
    return (((new_m & 2047) | exp_part) >> 1) | sign;
}

short f16_add(short a,short b)
{
    if (((a ^ b) & 0x8000) != 0)
        return f16_sub(a,b ^ 0x8000);
    short sign = a & 0x8000;
    a &= 0x7FFF;
    b &= 0x7FFF;
    if(a<b) {
        short x=a;
        a=b;
        b=x;
    }
    short ax = (a & 0x7C00);
    short bx = (b & 0x7C00);
    short exp_diff = ax - bx;
    short exp_part = ax;
    if(exp_diff != 0) {
        int shift = exp_diff >> 10;
        if(bx != 0)
            b = ((b & 1023) | 1024) >> shift;
        else
            b >>= (shift - 1);
    }
    else {
        if(bx == 0) {
            return (a + b) | sign;
        }
        else {
            b=(b & 1023) | 1024;
        }
    }
    short r=a+b;
    if ((r & 0x7C00) != exp_part) {
        unsigned short am = (a & 1023) | 1024;
        unsigned short new_m = (am + b) >> 1;
        r =( exp_part + 0x400) | (1023 & new_m);
    }
    if((unsigned short)r >= 0x7C00u) {
        return sign | 0x7C00;
    }
    return r | sign;
}


short f16_add_src(short a,short b)
{
    int op_add = ((a ^ b) & SIGN_MASK) == 0;
    unsigned short sign = a & SIGN_MASK;
    if(IS_ZERO(a)) {
        if(IS_ZERO(b))
            return 0;
        return b;
    }
    if(IS_ZERO(b))
        return a;
    if(IS_INVALID(a) || IS_INVALID(b))
        return NAN_VALUE;

    if((0x7FFF & a) < (0x7FFF & b)) {
        short x=a;
        a=b;
        b=x;
        if(!op_add)
            sign ^= SIGN_MASK;
    }
    unsigned short m1 = MANTISSA(a);
    unsigned short m2 = MANTISSA(b);
    int ax = EXPONENT(a);
    int bx = EXPONENT(b);
    int exp = ax;

    ax += (ax==0);
    bx += (bx==0);

    unsigned short diff = ax - bx;

    if(op_add) {
        if(diff > 0) {
            m2>>=diff;
        }
        m1 += m2;
        if(m1 & 2048) {
            m1 >>= 1;
            exp++;
        }
        if(exp == 0 && (m1 & 1024))
            exp=1;
    }
    else {
#define DIFF_SHIFT 1
        m1<<=DIFF_SHIFT;
        m2<<=DIFF_SHIFT;
        if(diff > 0) {
            m2>>=diff;
        }
        m1 -= m2;
        while(exp > 0 && !(m1 & (1024<<DIFF_SHIFT))) { // for shift 3
            exp--;
            if(exp!=0)
                m1<<=1;
        }
        m1>>=DIFF_SHIFT;
    }
    if(exp >= 31)
        return SIGNED_INF_VALUE(sign);
    
    a = sign | (exp << 10) | (m1 & 1023);
    return a;
}
short f16_sub_src(short a,short b)
{
    return f16_add_src(a,b^SIGN_MASK);
}

short f16_mul(short a,short b)
{
    int sign = (a ^ b) & SIGN_MASK;

    if(IS_INVALID(a) || IS_INVALID(b))
        return NAN_VALUE;

    if(IS_ZERO(a) || IS_ZERO(b))
        return 0;
    unsigned short m1 = MANTISSA(a);
    unsigned short m2 = MANTISSA(b);

    uint32_t v=m1;
    v*=m2;
    int ax = EXPONENT(a);
    int bx = EXPONENT(b);
    ax += (ax==0);
    bx += (bx==0);
    int new_exp = ax + bx - 15;
    
    if(v & ((uint32_t)1<<21)) {
        v >>= 11;
        new_exp++;
    }
    else if(v & ((uint32_t)1<<20)) {
        v >>= 10;
    }
    else { // denormal
        new_exp -= 10;
        while(v >= 2048) {
            v>>=1;
            new_exp++;
        }
    }
    if(new_exp <= 0) {
        v>>=(-new_exp + 1);
        new_exp = 0;
    }
    else if(new_exp >= 31) {
        return SIGNED_INF_VALUE(sign);
    }
    return (sign) | (new_exp << 10) | (v & 1023);
}

short f16_div(short a,short b)
{
    short sign = (a ^ b) & SIGN_MASK;
    if(IS_INVALID(a) || IS_INVALID(b) || (IS_ZERO(a) && IS_ZERO(b)))
        return NAN_VALUE;

    if(IS_ZERO(b) ) {
        return SIGNED_INF_VALUE(sign);
    }

    if(IS_ZERO(a))
        return 0;

    unsigned short m1 = MANTISSA(a);
    unsigned short m2 = MANTISSA(b);
#define DIV_SHIFT 3
    uint32_t v=(uint32_t)m1 << (10 + DIV_SHIFT);
    v= (v  + (m2>>1)) / m2;
    
    int ax = EXPONENT(a);
    int bx = EXPONENT(b);
    ax += (ax==0);
    bx += (bx==0);
    int new_exp = ax - bx + 15 ;

    if(v == 0)
        return 0;

    while(v < (1024<<DIV_SHIFT) && new_exp > 0) {
        v<<=1;
        new_exp--;
    }
    while(v > (2048<<DIV_SHIFT)) {
        v>>=1;
        new_exp++;
    }
    
    if(new_exp <= 0) {
        v>>=(-new_exp + 1);
        new_exp = 0;
    }
    else if(new_exp >= 31) {
        return SIGNED_INF_VALUE(sign);
    }
    v>>=DIV_SHIFT;
    return sign | (v & 1023) | (new_exp << 10);
}

short f16_neg(short v)
{
    return SIGN_MASK ^ v;
}
short f16_from_int(short v)
{
    if(v==0)
        return 0;
    int sig = 0;
    if(v < 0) {
        v=-v;
        sig=1;
    }
    int e=25;
    while(v >= 2048) {
        v>>=1;
        e++;
    }
    while(v<1024) {
        v<<=1;
        e--;
    }
    if(e>=31)
        return SIGNED_INF_VALUE(sig << 15);
    return (sig << 15) | (e << 10) | (v & 1023);
}
short f16_int(short a)
{
    unsigned short value = MANTISSA(a);
    short shift = EXPONENT(a) - 25;
    if(shift > 0)
        value <<= shift;
    else if(shift < 0)
        value >>= -shift;
    if(a & SIGN_MASK)
        return -(short)(value);
    return value;
}

int f16_gte(short a,short b)
{
    if(IS_ZERO(a) && IS_ZERO(b))
        return 1;
    if((a & SIGN_MASK) == 0) {
        if((b & SIGN_MASK) == SIGN_MASK)
            return 1;
        return a >= b;
    }
    else {
        if((b & SIGN_MASK) == 0)
            return 0;
        return (a & 0x7FFF) <= (b & 0x7FFF);
    }
}

int f16_gt(short a,short b)
{
    if(IS_ZERO(a) && IS_ZERO(b))
        return 0;
    if((a & SIGN_MASK) == 0) {
        if((b & SIGN_MASK) == SIGN_MASK)
            return 1;
        return a > b;
    }
    else {
        if((b & SIGN_MASK) == 0)
            return 0;
        return (a & 0x7FFF) < (b & 0x7FFF);
    }
    
}
int f16_eq(short a,short b)
{
    if(IS_ZERO(a) && IS_ZERO(b))
        return 1;
    return a==b;
}

int f16_lte(short a,short b)
{
    return f16_gte(b,a);
}

int f16_lt(short a,short b)
{
    return f16_gt(b,a);
}
int f16_neq(short a,short b)
{
    return !f16_eq(a,b);
}

char *f16_ftos(short a)
{
    static char buf[32];
    f16_ftosp(buf,sizeof(buf),a,5);
    return buf;
}

#define add_char(c) do { if(size>1) { *p++ = (c); size--; } } while(0)
#define add_str(s) do { char const *tmp=s; while(size>1 && *tmp) { *p++=*tmp++; size--; } } while(0)

#ifndef F16_NO_IO
char *f16_ftosp(char *p,int size,short a,int prec)
{
    char *save = p;
    unsigned short v;
    short f16_10 = f16_from_int(10);
    static char buf[8];
    if(a < 0) {
        add_char('-');
        a = a & 0x7FFF;
    }
    if(IS_INVALID(a)) {
        if((a & 1023) == 0)
            add_str("inf");
        else
            add_str("nan");
        goto done;
    }
    v = f16_int(a);
    sprintf(buf,"%d",v);
    add_str(buf);
    a=f16_sub_src(a,f16_from_int(v));
    if(a == 0 || prec==0)
        goto done;
    add_char('.');
    for(int i=0;i<prec && a != 0;i++) {
        a=f16_mul(a,f16_10);
        v=f16_int(a);
        a=f16_sub_src(a,f16_from_int(v));
        add_char(v+'0');
    }
done:
    *p=0;
    return save;    
}
#else
char *f16_ftosp(char *p,int size,short a,int prec)
{
    if(size > 0)
        *p=0;
}
#endif
