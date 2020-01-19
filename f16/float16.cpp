#include <assert.h>
#include <iostream>
#include <string.h>
#include <emmintrin.h>
#include <immintrin.h>
#include <stdio.h>
#include "float16.h"
#include "float16_impl.inl"
#include <fstream>


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
    tmp.m.value &= ~(unsigned)((1<<13)-1);
    int new_exp = tmp.m.exponent - 127 + 15;
    if(new_exp >=31)
        return float16_inf(tmp.m.sign);
    __m128 iv;
    memset(&iv,0,sizeof(iv));
    memcpy(&iv,&v,4);
    __m128i resv = _mm_cvtps_ph(iv,_MM_FROUND_TO_NEAREST_INT |_MM_FROUND_NO_EXC);
    float16_t res;
    memcpy(&res,&resv,2);
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
#if 1 
#include "half.hpp"
float16_t calc(int op,float16_t a,float16_t b)
{
    half_float::half v1,v2;
    memcpy(&v1,&a,2);
    memcpy(&v2,&b,2);
    switch(op) {
    case '+': v1+=v2; break;
    case '-': v1-=v2; break;
    case '*': v1*=v2; break;
    case '/': v1/=v2; break;
    case '>': v1 = v1 > v2; break;
    case '=': v1 = v1 == v2; break;
    case ':': v1 = v1 >= v2; break;
    }
    float16_t r;
    memcpy(&r,&v1,2);
    return r;
}

#else
float16_t calc(int op,float16_t a,float16_t b)
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
    return float32_to_float16(v1);
}
#endif
bool force_equal=true;


void test_op(char op,float16_t a,float16_t b)
{
    float16_t ref = calc(op,a,b);
    float16_t r = float16_t();
    switch(op) {
    case '+': r=float16_add(a,b); break;
    case '-': r=float16_sub(a,b); break;
    case '*': r=float16_mul(a,b); break;
    case '/': r=float16_div(a,b); break;
    case '>': r=int_to_float16(float16_gt(a,b)); break;
    case '=': r=int_to_float16(float16_eq(a,b)); break;
    case ':': r=int_to_float16(float16_gte(a,b)); break;
    }
#if 1    
    unsigned short rx = 0;
    unsigned short ax=a.m.value,bx=b.m.value;
    switch(op) {
    case '+': rx=f16_add(ax,bx); break;
    case '-': rx=f16_sub(ax,bx); break;
    case '*': rx=f16_mul(ax,bx); break;
    case '/': rx=f16_div(ax,bx); break;
    case '>': rx=f16_from_int(f16_gt(ax,bx)); break;
    case '=': rx=f16_from_int(f16_eq(ax,bx)); break;
    case ':': rx=f16_from_int(f16_gte(ax,bx)); break;
    }
    if(rand() % 1000 == 0) {
        std::ofstream tmps("/tmp/samples.txt",std::ofstream::app);
        tmps << "{'"<<op<<"'," <<ax<<","<<bx<<","<<rx<<"},"<<std::endl;
    }

    if(rx != r.m.value) {
        char buf[64];
        printf("Operation %s(%d) %c ",float16_format(buf,sizeof(buf),a,8),ax,op);
        printf("%s(%d) =  ",float16_format(buf,sizeof(buf),b,8),bx);
        printf("%s(%d) !=  ",float16_format(buf,sizeof(buf),r,8),r.m.value);
        printf("%s(%d)\n",f16_ftosp(buf,sizeof(buf),rx,8),rx);
        exit(1);
    }
#endif    

    float res = float16_to_float32(r);
    if(!float16_eq(r,ref)) {
        if(force_equal || r.m.sign != ref.m.sign) {
            std::cout << "Failed for " << float16_to_float32(a) << op << float16_to_float32(b)  << " =" <<ref << " != " << res << std::endl;
            return;
        }

        int diff = labs((int)(r.m.value & 0x7FFF) - (int)(ref.m.value & 0x7FFF));
        if(diff > 1) {
            std::cout << "Failed for "<< float16_to_float32(a)  << "("<< a.m.value<<")" << op << float16_to_float32(b)<<"(" << b.m.value << ")"  << " =" <<ref << " not " << res << " by " << diff << std::endl;
        }
        
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
        if((s1 & 0x7C00) == 0x7C00 || (s2 & 0x7C00) == 0x7C00){
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

void test_print(float v)
{
    char buf[128];
    char buf2[128];
    float16_t tmp = float32_to_float16(v);
    v=float16_to_float32(tmp);
    float16_format(buf,sizeof(buf),tmp,15);
    f16_ftosp(buf2,sizeof(buf2),tmp.m.value,15);
    printf("%f=%s\n",v,buf);
    assert(strcmp(buf,buf2)==0);
}
static short fix_sign(short r)
{
    if(r<0) {
        r=-r | 0x8000;
    }
    return r;
}
void test_subnormals()
{
    for(short i=-2047;i<2048;) {
        for(short j=-2047;j<2048;) {
            short si=fix_sign(i),sj=fix_sign(j);
            float16_t a,b;
            a.m.value = si;
            b.m.value = sj;
            test_op('+',a,b);
            test_op('-',a,b);
            if(j<-1000 || 1000<j)
                j+=100;
            else if(j<100 || 100<j)
                j+=10;
            else
                j++;
        }
        if(i<-1000 || 1000<i)
            i+=100;
        else if(i<100 || 100<i)
            i+=10;
        else
            i++;
    }
}

int main()
{
//    printf("Testing!\n");
//    test(947,947);
//    return 0;
    printf("Subnorm!\n");
    test_subnormals();
    printf("Done subnorm\n");
    test0();
    test1();
    force_equal = false;
    test(947,937);
    test(8200,8145);
    test(7915,7927);
    test(10958,466);
    test(38629,40509);
    test(38591,62587);
    test(22696,7903);
    test(9184,7903);
    test(62945,33282);
    test(32959,37477);
    test(9184,7903);
    test2();
    test3();
    printf("One=%d %d\n",int_to_float16(1).m.value,float32_to_float16(1.0f).m.value);
    printf("%f\n",float16_to_float32(float16_div(int_to_float16(5),int_to_float16(10000))));
    test_print(123.234);
    test_print(-1024);
    test_print(123423323);
    test_print(0.2145232);
    test_print(0.0005);
    return 0;
}
