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

#define IS_ZERO(v) (((v).m.value & 0x7FFF) == 0)
#define IS_INVALID(v) ((v).m.exponent == 31)
#define MANTISSA(v) ((v).m.exponent == 0 ? ((v).m.fraction) :( 1024 | (v).m.fraction))

static float16_t float16_sub(float16_t a,float16_t b);
static float16_t float16_add(float16_t a,float16_t b);

static float16_t float16_add(float16_t a,float16_t b)
{
    if(IS_INVALID(a) || IS_INVALID(b))
        return float16_nan();

    int op_add = a.m.sign == b.m.sign;
    int sign = a.m.sign;
    if((0x7FFF & a.m.value) < (0x7FFF & b.m.value)) {
        float16_t x=a;
        a=b;
        b=x;
        if(!op_add)
            sign ^= 1;
    }
    int exp = a.m.exponent;
    unsigned short m1 = MANTISSA(a);
    unsigned short m2 = MANTISSA(b);
    int ax = a.m.exponent;
    int bx = b.m.exponent;
    ax += (ax==0);
    bx += (bx==0);

    unsigned short diff = ax - bx;

    //printf("ax=%d(%d) bx=%d(%d) m1=%d m2=%d diff=%d\n",ax,a.m.exponent,bx,b.m.exponent,m1,m2,diff);

    if(op_add) {
        if(diff > 0) {
            m2>>=diff;
        }
        m1 += m2;
        if(m1 & 2048) {
            m1 >>= 1;
            exp++;
        }
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
            m1<<=1;
            exp--;
        }
        if(exp == 0)
            m1>>=1 + DIFF_SHIFT;
        else
            m1>>=DIFF_SHIFT;
        //printf("final M=%d e=%d\n",m1,exp);
    }
    if(exp >= 31)
        return float16_inf(sign);
    a.m.exponent = exp;
    a.m.fraction = m1 & 1023;
    a.m.sign = sign;
    return a;
}

static float16_t float16_sub(float16_t a,float16_t b)
{
    return float16_add(a,float16_neg(b));
}



static float16_t float16_mul(float16_t a,float16_t b)
{
    if(IS_INVALID(a) || IS_INVALID(b))
        return float16_nan();

    int sign = a.m.sign ^ b.m.sign;

    unsigned short m1 = MANTISSA(a);
    unsigned short m2 = MANTISSA(b);

    if(m1 == 0 || m2 == 0)
        return float16_zero(sign);

    uint32_t v=m1;
    v*=m2;
    int ax = a.m.exponent;
    int bx = b.m.exponent;
    ax += (ax==0);
    bx += (bx==0);
    int new_exp = ax + bx - 15;
    
    if(v & (1<<21)) {
        v >>= 11;
        new_exp++;
    }
    else if(v & (1<<20)) {
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
        return float16_inf(sign);
    }
    assert((new_exp > 0 && (1024<=v && v<2048)) || (new_exp == 0 && v<1024));
    float16_t res;
    res.m.fraction = v & 1023;
    res.m.exponent = new_exp;
    res.m.sign = sign;
    return res;
}

static float16_t float16_div(float16_t a,float16_t b)
{
    int sign = a.m.sign ^ b.m.sign;
    if(IS_INVALID(a) || IS_INVALID(b))
        return float16_nan();
    if(IS_ZERO(b)) {
        return float16_inf(sign);
    }

    if(IS_ZERO(a))
        return float16_zero(0);

    unsigned short m1 = MANTISSA(a);
    unsigned short m2 = MANTISSA(b);
#define DIV_SHIFT 3
    uint32_t v=(unsigned)m1 << (10 + DIV_SHIFT);
    v= (v  + (m2>>1)) / m2;
    
    int ax = a.m.exponent;
    int bx = b.m.exponent;
    ax += (ax==0);
    bx += (bx==0);
    int new_exp = ax - bx + 15 ;
//  printf("ax=%d(%d) bx=%d(%d) m1=%d m2=%d new_exp=%d v=%d\n",ax,a.m.exponent,bx,b.m.exponent,m1,m2,new_exp,v);


    if(v == 0)
        return float16_zero(sign);

    while(v < (1024<<DIV_SHIFT) && new_exp > 0) {
        v<<=1;
        new_exp--;
    }
    while(v > (2048<<DIV_SHIFT)) {
        v>>=1;
        new_exp++;
    }
  //printf("v=%d exp=%d\n",v,new_exp);
    
    if(new_exp <= 0) {
        v>>=(-new_exp + 1);
        new_exp = 0;
    }
    else if(new_exp >= 31) {
        return float16_inf(sign);
    }
    v>>=DIV_SHIFT;
    float16_t res;
    res.m.sign = sign;
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
    if(IS_ZERO(a) && IS_ZERO(b))
        return 1;
    return a.m.value == b.m.value;
}

int float16_gt(float16_t a, float16_t b)
{
    if(IS_ZERO(a) && IS_ZERO(b))
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
    if(IS_ZERO(a) && IS_ZERO(b))
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
