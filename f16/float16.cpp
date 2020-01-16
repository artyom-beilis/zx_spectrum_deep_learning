#include <assert.h>
#include <iostream>
#include <string.h>
#include <emmintrin.h>
#include <immintrin.h>
#include <stdio.h>
#include "float16.h"
#include "float16_impl.i"


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
    case '>': v1 = v1 > v2; break;
    case '=': v1 = v1 == v2; break;
    case ':': v1 = v1 >= v2; break;
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
    case '>': r=int_to_float16(float16_gt(a,b)); break;
    case '=': r=int_to_float16(float16_eq(a,b)); break;
    case ':': r=int_to_float16(float16_gte(a,b)); break;
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


void test(unsigned short s1,unsigned short s2,bool with_mul=true)
{
    float16_t a,b;
    memcpy(&a,&s1,2);
    memcpy(&b,&s2,2);
    test_op('+',a,b);
    test_op('-',a,b);
    test_op('=',a,b);
    test_op('>',a,b);
    test_op(':',a,b);
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

void test_int(float v)
{
    int iv=int(v);
    int iv2 = float16_int(float32_to_float16(v));
    if(iv2 != iv) {
        std::cout << v << " int:" << iv << "!=" << iv2 << std::endl;
    }
    int iv3 = float16_int(float32_to_float16(iv));
    if(iv!=iv3) {
        std::cout << v << " int:" << iv << "!=" << iv3 << std::endl;
    }
}



void test0()
{
    test_int(135.13);
    test_int(1235.13);
    test_int(0.231);
    test_int(0.9);
    test_int(0.01);
    test_int(1023.243);
    test_int(4000.124);

    test_int(-135.13);
    test_int(-1235.13);
    test_int(-0.231);
    test_int(-0.9);
    test_int(-0.01);
    test_int(-1023.243);
    test_int(-4000.124);


}

int main()
{
    test0();
    test1();
    force_equal = false;
    test(22696,7903);
    test(9184,7903);
    test2();
    test3();
    printf("One=%d %d\n",int_to_float16(1).m.value,float32_to_float16(1.0f).m.value);
    return 0;
}
