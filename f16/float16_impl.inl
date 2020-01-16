//#define F16_ROUND
#include <stdio.h>
#include <stdint.h>
typedef struct Float16 {
    union {
        unsigned short value;
        struct {
            unsigned short fraction : 10;
            unsigned short exponent : 5;
            unsigned short sign : 1;
        };
    } m;
} float16_t;



static float16_t float16_inf(int neg)
{
    float16_t r;
    r.m.sign = neg;
    r.m.fraction = 0;
    r.m.exponent = 31;
    return r;
}

static float16_t float16_nan()
{
    float16_t r;
    r.m.sign = 0;
    r.m.value = 1023;
    r.m.exponent = 31;
    return r;
}


static float16_t float16_zero(int neg)
{
    float16_t r;
    r.m.sign = neg;
    r.m.value = 0;
    r.m.exponent = 0;
    return r;
}

static float16_t float16_neg(float16_t v)
{
    v.m.sign ^= 1;
    return v;
}

#define IS_INVALID(v) ((v).m.exponent == 31)

static float16_t float16_sub(float16_t a,float16_t b);
static float16_t float16_add(float16_t a,float16_t b);

static float16_t float16_add(float16_t a,float16_t b)
{
    if(a.m.sign != b.m.sign) {
        b.m.sign = 1 ^ b.m.sign;
        return float16_sub(a,b);
    };
    if(a.m.exponent == 0)
        return b;
    if(b.m.exponent == 0)
        return a;
    if(IS_INVALID(a) || IS_INVALID(b))
        return float16_nan();

    if((0x7FFF & a.m.value) < (0x7FFF & b.m.value)) {
        float16_t x=a;
        a=b;
        b=x;
    }
    unsigned short m1 = a.m.fraction | 1024;
    unsigned short m2 = b.m.fraction | 1024;
    unsigned short diff = a.m.exponent - b.m.exponent;
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
        a.m.exponent++;
        if(a.m.exponent == 31) {
            a.m.fraction = 0;
            return a;
        }
    }
    a.m.fraction = m1 & 1023;
    return a;
}

static float16_t float16_sub(float16_t a,float16_t b)
{
    if(a.m.exponent == 0) 
        return float16_neg(b);
    if(b.m.exponent == 0)
        return a;

    if(IS_INVALID(a) || IS_INVALID(b))
        return float16_nan();

    if(a.m.sign != b.m.sign) {
        b.m.sign^=1;
        return float16_add(a,b);
    };

    if((a.m.value & 0x7FFF) < (b.m.value & 0x7FFF)) {
        float16_t x=a;
        a=b;
        b=x;
        a.m.sign ^= 1;
    }
    
    unsigned ediff = a.m.exponent - b.m.exponent;
    unsigned m1 = a.m.fraction | 1024;
    unsigned m2 = b.m.fraction | 1024;
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
        int new_exp = a.m.exponent;
        while(!(m1 & 1024)) {
            new_exp --;
            m1 <<= 1;
        }
        if(new_exp <= 0)
            return float16_zero(0);
        a.m.exponent = new_exp;
        a.m.fraction = m1 & 1023;
        return a;
    }
    return float16_zero(0);
}



static float16_t float16_mul(float16_t a,float16_t b)
{
    float16_t res;
    res.m.sign = a.m.sign ^ b.m.sign;

    if(IS_INVALID(a) || IS_INVALID(b))
        return float16_nan();

    if(a.m.exponent == 0 || b.m.exponent == 0)
        return float16_zero(0);

    unsigned short m1 = a.m.fraction | 1024;
    unsigned short m2 = b.m.fraction | 1024;
    uint32_t v=m1;
    v*=m2;
    int new_exp = a.m.exponent + b.m.exponent - 15;
    
    if(v & (1<<21)) {
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
        return float16_zero(res.m.sign);
    }
    if(new_exp >= 31) {
        return float16_inf(res.m.sign);
    }
    res.m.fraction = v & 1023;
    res.m.exponent = new_exp;
    return res;
}

static float16_t float16_div(float16_t a,float16_t b)
{
    float16_t res;
    res.m.sign = a.m.sign ^ b.m.sign;
    if(IS_INVALID(a) || IS_INVALID(b))
        return float16_nan();
    if(b.m.exponent == 0) {
        return float16_inf(res.m.sign);
    }

    if(a.m.exponent == 0)
        return float16_zero(0);

    unsigned short m1 = a.m.fraction | 1024;
    unsigned short m2 = b.m.fraction | 1024;
    uint32_t v=(unsigned)m1 << 10;
#ifdef F16_ROUND        
    v= (v + (m2>>1)) / m2;
#else
    v= v / m2;
#endif    
    
    int new_exp = a.m.exponent - b.m.exponent + 15;

    
    if(!(v & 1024)) {
        v<<=1;
        new_exp--;
    }
    if(new_exp <= 0) {
        return float16_zero(res.m.sign);
    }
    if(new_exp >= 31) {
        return float16_inf(res.m.sign);
    }
    res.m.fraction = v & 1023;
    res.m.exponent = new_exp;
    return res;
}


static short float16_int(float16_t a)
{
    unsigned short value = a.m.fraction | 1024;
    short shift = a.m.exponent - 25;
    if(shift > 0)
        value <<= shift;
    else if(shift < 0)
        value >>= -shift;
    if(a.m.sign)
        return -(short)(value);
    return value;
}

static float16_t int_to_float16(short v)
{
    if(v==0) {
        return float16_zero(0);
    }
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
        return float16_inf(sig);
    float16_t r;
    r.m.sign = sig;
    r.m.exponent = e;
    r.m.fraction = v & 1023;
    return r;
}

int float16_eq(float16_t a, float16_t b)
{
    if(a.m.exponent == 0 && b.m.exponent == 0)
        return 1;
    return a.m.value == b.m.value;
}

int float16_gt(float16_t a, float16_t b)
{
    if(a.m.exponent == 0 && b.m.exponent == 0)
        return 0;
    if(a.m.sign == 0) {
        if(b.m.sign == 1)
            return 1;
        return a.m.value > b.m.value;
    }
    else {
        if(b.m.sign == 0)
            return 0;
        return (a.m.value & 0x7FFF) < (b.m.value & 0x7FFF);
    }
}

int float16_gte(float16_t a, float16_t b)
{
    if(a.m.exponent == 0 && b.m.exponent == 0)
        return 1;
    if(a.m.sign == 0) {
        if(b.m.sign == 1)
            return 1;
        return a.m.value >= b.m.value;
    }
    else {
        if(b.m.sign == 0)
            return 0;
        return (a.m.value & 0x7FFF) <= (b.m.value & 0x7FFF);
    }
}

#define add_char(c) do { if(size>1) { *p++ = (c); size--; } } while(0)
#define add_str(s) do { char const *tmp=s; while(size>1 && *tmp) { *p++=*tmp++; size--; } } while(0)

static char *float16_format(char *p,int size,float16_t a,int prec)
{
    char *save = p;
    unsigned short v;
    static char buf[8];
    if(a.m.sign) {
        add_char('-');
        a.m.sign = 0;
    }
    if(a.m.exponent == 31) {
        if(a.m.fraction == 0)
            add_str("inf");
        else
            add_str("nan");
        goto done;
    }
    v = float16_int(a);
    sprintf(buf,"%d",v);
    add_str(buf);
    a=float16_sub(a,int_to_float16(v));
    if(a.m.value == 0 || prec==0)
        goto done;
    add_char('.');
    for(int i=0;i<prec && a.m.value != 0;i++) {
        a=float16_mul(a,int_to_float16(10));
        v=float16_int(a);
        a=float16_sub(a,int_to_float16(v));
        add_char(v+'0');
    }
done:
    *p=0;
    return save;    
}
