#include "float16.h"
#include <stdint.h>
#include <stdio.h>

#define SIGN_MASK 0x8000
#define EXP_MASK 0x7C00
#define NAN_VALUE 0x7FFF
#define IS_ZERO(x) (((x) & EXP_MASK) == 0)
#define IS_INVALID(x) (((x) & EXP_MASK) == EXP_MASK)
#define MANTISSA(x) (((x) & 1023) | 1024)
#define EXPONENT(x) (((x) & 0x7C00) >> 10)
#define SIGNED_INF_VALUE(x)  ((x & SIGN_MASK) | 0x7C00)

short f16_add(short a,short b)
{
    if(((a ^ b) & SIGN_MASK) == SIGN_MASK) {
        b^=0x8000;
        return f16_sub(a,b);
    }
    if(IS_ZERO(a))
        return b;
    if(IS_ZERO(b))
        return a;
    if(IS_INVALID(a) || IS_INVALID(b))
        return NAN_VALUE;

    if((0x7FFF & a) < (0x7FFF & b)) {
        short x=a;
        a=b;
        b=x;
    }
    unsigned short m1 = MANTISSA(a);
    unsigned short m2 = MANTISSA(b);
    int ax = EXPONENT(a);
    int bx = EXPONENT(b);
    unsigned short diff = ax - bx;
    if(diff > 0) {
#ifdef F16_ROUND
        m2 >>= diff-1;
        m2++;
        m2 >>= 1;
#else
        m2>>=diff;
#endif
    }
    m1 += m2;
    if(m1 & 2048) {
        m1>>=1;
        ax++;
        if(ax == 31) {
            return SIGNED_INF_VALUE(a);
        }
    }
    a = (a & SIGN_MASK) | (ax << 10) | (m1 & 1023);
    return a;
}
short f16_sub(short a,short b)
{
    if(IS_ZERO(a)) 
        return b ^ SIGN_MASK;
    if(IS_ZERO(b))
        return a;

    if(IS_INVALID(a) || IS_INVALID(b))
        return NAN_VALUE;

    if(((a ^ b) & SIGN_MASK)==SIGN_MASK) {
        b^=SIGN_MASK;
        return f16_add(a,b);
    };

    if((a & 0x7FFF) < (b & 0x7FFF)) {
        short x=a;
        a=b;
        b=x;
        a^= SIGN_MASK;
    }
    
    int ax = EXPONENT(a);
    int bx = EXPONENT(b); 
    unsigned ediff = ax - bx;
    unsigned m1 = MANTISSA(a);
    unsigned m2 = MANTISSA(b);
    if(ediff) {
#ifdef F16_ROUND        
        m2 >>= ediff-1;
        m2++;
        m2 >>=1;
#else
        m2 >>= ediff;
#endif        
    }
    m1 -= m2;
    if(m1 != 0) {
        int new_exp = ax;
        while(!(m1 & 1024)) {
            new_exp --;
            m1 <<= 1;
        }
        if(new_exp <= 0)
            return 0;
        a = (a & SIGN_MASK) | (m1 & 1023) | (new_exp << 10);
        return a;
    }
    return 0;
}
short f16_mul(short a,short b)
{
    short res = (a ^ b) & SIGN_MASK;

    if(IS_INVALID(a) || IS_INVALID(b))
        return NAN_VALUE;

    if(IS_ZERO(a) || IS_ZERO(b))
        return 0;

    unsigned short m1 = MANTISSA(a);
    unsigned short m2 = MANTISSA(b);
    uint32_t v=m1;
    v*=m2;
    int new_exp = EXPONENT(a) + EXPONENT(b) - 15;
    
    if(v & ((uint32_t)1<<21)) {
#ifdef F16_ROUND
        v+= 1024;
#endif        
        v >>= 11;
        new_exp++;
    }
    else {
#ifdef F16_ROUND        
        v+=512;
#endif        
        v >>= 10;
    }

#ifdef F16_ROUND        
    if(v & 2048) {
        v >>= 1;
        new_exp++;
    }
#endif    
    if(new_exp <= 0) {
        return 0;
    }
    if(new_exp >= 31) {
        return SIGNED_INF_VALUE(res);
    }
    return (res & SIGN_MASK) | (new_exp << 10) | (v & 1023);
}

short f16_div(short a,short b)
{
    short res = (a ^ b) & SIGN_MASK;
    if(IS_INVALID(a) || IS_INVALID(b))
        return NAN_VALUE;
    if(IS_ZERO(b)) {
        return SIGNED_INF_VALUE(res);
    }

    if(IS_ZERO(a))
        return 0;

    unsigned short m1 = MANTISSA(a);
    unsigned short m2 = MANTISSA(b);
    uint32_t v=m1;
    v<<=10;
#ifdef F16_ROUND        
    v= (v + (m2>>1)) / m2;
#else
    v= v / m2;
#endif    
    
    int new_exp = EXPONENT(a) - EXPONENT(b) + 15;

    
    if(!(v & 1024)) {
        v<<=1;
        new_exp--;
    }
    if(new_exp <= 0) {
        return 0;
    }
    if(new_exp >= 31) {
        return SIGNED_INF_VALUE(res);
    }
    return res | (v & 1023) | (new_exp << 10);
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
    a=f16_sub(a,f16_from_int(v));
    if(a == 0 || prec==0)
        goto done;
    add_char('.');
    for(int i=0;i<prec && a != 0;i++) {
        a=f16_mul(a,f16_10);
        v=f16_int(a);
        a=f16_sub(a,f16_from_int(v));
        add_char(v+'0');
    }
done:
    *p=0;
    return save;    
}
