#include <assert.h>
#include <iostream>
#include <string.h>
#include <emmintrin.h>
#include <immintrin.h>
#define FLOAT16_BIAS 15
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



typedef struct Float32 {
    union {
        unsigned value;
        float fvalue;
        struct {
            unsigned fraction: 23;
            unsigned exponent: 8;
            unsigned sign: 1;
        };
    } m;
} Float32;

float16_t float16_inf(int neg)
{
    float16_t r;
    r.m.sign = neg;
    r.m.value = 0;
    r.m.exponent = 31;
    return r;
}

float16_t float16_zero(int neg)
{
    float16_t r;
    r.m.sign = neg;
    r.m.value = 0;
    r.m.exponent = 0;
    return r;
}

float16_t float32_to_float16(float v)
{
    Float32 tmp;
    tmp.m.fvalue = v;
    int new_exp = tmp.m.exponent - 127 + 15;
    if(new_exp <= 0)
        return float16_zero(tmp.m.sign);
    if(new_exp >=31)
        return float16_inf(tmp.m.sign);
    float16_t res;
    res.m.sign = tmp.m.sign;
    res.m.exponent = new_exp;
    res.m.fraction = tmp.m.fraction >> 13;
    //__m128 iv;
    //memset(&iv,0,sizeof(iv));
    //memcpy(&iv,&v,4);
    //__m128i resv = _mm_cvtps_ph(iv,_MM_FROUND_TO_ZERO |_MM_FROUND_NO_EXC);
    //float16_t res;
    //memcpy(&res,&resv,2);
    return res;
}


float float16_to_float32(float16_t v)
{
    __m128i iv;
    memset(&iv,0,sizeof(iv));
    memcpy(&iv,&v,2);
    __m128 resv = _mm_cvtph_ps(iv);
    float res;
    memcpy(&res,&resv,4);
    return res;
}

float16_t float16_neg(float16_t v)
{
    v.m.sign ^= 1;
    return v;
}

float16_t float16_sub(float16_t a,float16_t b);
float16_t float16_add(float16_t a,float16_t b);

float16_t float16_add(float16_t a,float16_t b)
{
    if(a.m.sign != b.m.sign) {
        b.m.sign = 1 ^ b.m.sign;
        return float16_sub(a,b);
    };
    if(a.m.exponent == 0)
        return b;
    if(b.m.exponent == 0)
        return a;

    if((0x7FFF & a.m.value) < (0x7FFF & b.m.value)) {
        std::swap(a,b);
    }
    unsigned short m1 = a.m.fraction | 1024;
    unsigned short m2 = b.m.fraction | 1024;
    unsigned short diff = a.m.exponent - b.m.exponent;
    if(diff > 0) {
        m2 >>= diff-1;
        m2++;
        m2 >>= 1;
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

float16_t float16_sub(float16_t a,float16_t b)
{
    if(a.m.exponent == 0) 
        return float16_neg(b);
    if(b.m.exponent == 0)
        return a;

    if(a.m.sign != b.m.sign) {
        b.m.sign^=1;
        return float16_add(a,b);
    };

    if((a.m.value & 0x7FFF) < (b.m.value & 0x7FFF)) {
        std::swap(a,b);
        a.m.sign ^= 1;
    }
    
    unsigned ediff = a.m.exponent - b.m.exponent;
    unsigned m1 = a.m.fraction | 1024;
    unsigned m2 = b.m.fraction | 1024;
    if(ediff) {
        m2 >>= ediff-1;
        m2++;
        m2 >>=1;
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



float16_t float16_mul(float16_t a,float16_t b)
{
    float16_t res;
    res.m.sign = a.m.sign ^ b.m.sign;
    if(a.m.exponent == 0 || b.m.exponent == 0)
        return float16_zero(0);

    unsigned short m1 = a.m.fraction | 1024;
    unsigned short m2 = b.m.fraction | 1024;
    unsigned v=m1;
    v*=m2;
    int new_exp = a.m.exponent + b.m.exponent - 15;
    
    if(v & (1<<21)) {
        v+= 1024;
        v >>= 11;
        new_exp++;
    }
    else {
        v+=512;
        v >>= 10;
    }

    if(v & 2048) {
        v >>= 1;
        new_exp++;
    }
    assert(1024 <= v && v<2048);
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

float16_t float16_div(float16_t a,float16_t b)
{
    float16_t res;
    res.m.sign = a.m.sign ^ b.m.sign;
    if(b.m.exponent == 0) {
        return float16_inf(res.m.sign);
    }

    if(a.m.exponent == 0)
        return float16_zero(0);

    unsigned short m1 = a.m.fraction | 1024;
    unsigned short m2 = b.m.fraction | 1024;
    unsigned v=(unsigned)m1 << 10;
    v= (v + (m2>>1)) / m2;
    int new_exp = a.m.exponent - b.m.exponent + 15;
    
    if(!(v & 1024)) {
        v<<=1;
        new_exp--;
    }
    assert(1024<=v && v<2048);
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


std::ostream &operator<<(std::ostream &o,float16_t const &v)
{
    float v32=float16_to_float32(v);
    o << v32;
    return o;
}

unsigned short randv(unsigned short &lfsr)
{
    unsigned short bit;
    bit = ((lfsr >> 0) ^ (lfsr >> 2) ^ (lfsr >> 3) ^ (lfsr >> 5));
    lfsr = (lfsr >> 1) | (bit << 15);
    return lfsr;
}

float calc(int op,float16_t a,float16_t b)
{
    float v1=float16_to_float32(a);
    float v2=float16_to_float32(b);
    switch(op) {
    case '+': v1+=v2; break;
    case '-': v1-=v2; break;
    case '*': v1*=v2; break;
    case '/': v1/=v2; break;
    }
    return float16_to_float32(float32_to_float16(v1));
}

bool force_equal=true;

float16_t float16_abs_next(float16_t r)
{
    if(r.m.fraction == 1023) {
        if(r.m.exponent < 30) {
            r.m.exponent ++;
            r.m.fraction = 0;
        }
        else {
            r.m.exponent = 31;
            r.m.fraction = 0;
        }
    }
    else {
        r.m.fraction++;
    }
    return r;
}

float16_t float16_abs_prev(float16_t r)
{
    if(r.m.fraction == 0) {
        if(r.m.exponent > 1) {
            r.m.exponent --;
            r.m.fraction = 1023;
        }
        else {
            r.m.exponent = 0;
        }
    }
    else {
        r.m.fraction--;
    }
    return r;
}

float16_t float16_prev(float16_t r)
{
    if(r.m.sign)
        return float16_abs_next(r);
    else
        return float16_abs_prev(r);    
}

float16_t float16_next(float16_t r)
{
    if(r.m.sign)
        return float16_abs_prev(r);
    else
        return float16_abs_next(r);    
}


void test_op(char op,float16_t a,float16_t b)
{
    float ref = calc(op,a,b);
    float16_t r;
    switch(op) {
    case '+': r=float16_add(a,b); break;
    case '-': r=float16_sub(a,b); break;
    case '*': r=float16_mul(a,b); break;
    case '/': r=float16_div(a,b); break;
    }
    float res = float16_to_float32(r);
    if(ref != res) {
        if(force_equal) {
            std::cout << "Failed for " << float16_to_float32(a) << op << float16_to_float32(b)  << " =" <<ref << " != " << res << std::endl;
            return;
        }
        float16_t b16=r,a16=r;
        int epsN=5;
        if(op=='-' || op=='+') {
            float16_t bb=b;
            if(op=='-')
                bb.m.sign ^=1;
            if(bb.m.sign != a.m.sign) {
                int ediff = bb.m.exponent - a.m.exponent;
                if(ediff < 0)
                    ediff=-ediff;
                if(ediff == 0)
                    epsN = 35;
                else if(ediff == 1)
                    epsN = 32;
                //else if(ediff <= 5)
                //    epsN = 20;
                //else
                //    epsN = 10;
                //epsN=35;
            }
        }
        for(int i=0;i<epsN;i++) {
            b16 = float16_prev(b16);
            a16 = float16_next(a16);
        }
        float before = float16_to_float32(b16);
        float after  = float16_to_float32(a16);
        if(before <= ref && ref <=after)
            return;
        std::cout << "Failed for "<< float16_to_float32(a)  << "("<< a.m.value<<")" << op << float16_to_float32(b)<<"(" << b.m.value << ")"  << " =" <<ref << " not in [" << before << "," << after << "] = res" << res << std::endl;
    }
}

short float16_int(float16_t a)
{
    short value = a.m.fraction | 1024;
    if(a.m.exponent > 15)
        value <<= a.m.exponent - 15;
    else if(a.m.exponent < 15)
        value >>= 15 - a.m.exponent;
    if(a.m.sign)
        value = -value;
    return value;
}

void test(unsigned short s1,unsigned short s2,bool with_mul=true)
{
    float16_t a,b;
    memcpy(&a,&s1,2);
    memcpy(&b,&s2,2);
    test_op('+',a,b);
    test_op('-',a,b);
    if(with_mul) {
        test_op('*',a,b);
        test_op('/',a,b);
    }
}

void testf(float s1,float s2,bool with_mul=true)
{
    test(float32_to_float16(s1).m.value,float32_to_float16(s2).m.value,with_mul);
}


void test1()
{
    printf("Test Integers\n");
    for(int i=-1024;i<1024;i++) {
        for(int j=-1024;j<1024;j++) {
            testf(i,j,false);
            if(j != 0 && i % j == 0)
                test_op('/',float32_to_float16(i),float32_to_float16(j));
        }
    }
    for(int i=-45;i<45;i++) {
        for(int j=-45;j<45;j++) {
            test_op('*',float32_to_float16(i),float32_to_float16(j));
        }
    }
}
void test2()
{
    for(int i=-150;i<150;i++) {
        for(int j=-150;j<150;j++) {
            if(j!=0) {
                test(float32_to_float16(i).m.value,float32_to_float16(1.0f/j).m.value);
            }
            if(i!=0) {
                test(float32_to_float16(1.0f/i).m.value,float32_to_float16(j).m.value);
            }
            if(i!=0 && j!=0) {
                test(float32_to_float16(1.0f/i).m.value,float32_to_float16(1.0f/j).m.value);
            }
        }
    }
}
void test3()
{
    unsigned short s1=0xACE1;
    unsigned short s2=0x1d24;


    for(int i=0;i<1000;i++) {
        randv(s1);
        randv(s2);
        if((s1 & 0x7C00) == 0x7C00 || (s2 & 0x7C00) == 0x7C00 || (s1!=0&& (s1 & 0x7C00) == 0) || (s2!=0 && (s2 & 0x7C00) == 0)) {
            i--;
            continue;
        }
        test(s1,s2);
    }
}


int main()
{
    test1();
    force_equal = false;
    test(22696,7903);
    test(9184,7903);
    test2();
    test3();
    return 0;
}
