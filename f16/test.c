#include "float16.h"
#include <stdio.h>

unsigned short samples[500][4] = {
#include "ref.h"
};

extern short f16_add2(short a,short b);

int test_op(int n)
{
    unsigned short ax=samples[n][1];
    unsigned short bx=samples[n][2];
    unsigned short r=samples[n][3];
    unsigned short rx=0;
    char op = samples[n][0];
    //printf("Testing %c\n",op);
    switch(op) {
    case '+': rx=f16_add(ax,bx); break;
    case '-': rx=f16_sub(ax,bx); break;
    case '*': rx=f16_mul(ax,bx); break;
    case '/': rx=f16_div(ax,bx); break;
    case '>': rx=f16_from_int(f16_gt(ax,bx)); break;
    case '=': rx=f16_from_int(f16_eq(ax,bx)); break;
    case ':': rx=f16_from_int(f16_gte(ax,bx)); break;
    }
    if(f16_neq(rx,r)) {
        printf("%s %c ",f16_ftos(ax),op);
        printf("%s = ",f16_ftos(bx));
        printf("%s != ",f16_ftos(r));
        printf("%s\n ",f16_ftos(rx));
        printf("Test %u: %u %c %u = %u != %u\n",n,ax,op,bx,r,rx);
        return 0;
    }
    return 1;
}

void test(int a,int b,int exp)
{
    samples[0][0]='+';
    samples[0][1]=a;
    samples[0][2]=b;
    samples[0][3]=exp;
    test_op(0);
}

int main()
{
    int i,j,v,vref;
    //test(58002,25182,52864);
    //return 0;
/*    for(i=-15;i<15;i++) {
        for(j=-15;j<15;j++) {
            v=f16_add(f16_from_int(i),f16_from_int(j));
            vref = f16_from_int(i+j);
            if(v!=vref) {
                printf("%d + %d -> %x != %x\n",i,j,v,vref);
                return 1;
            }
        }
    }
    for(i=0;i<0x7C00;i++) {
        for(j=0;j<0x7C00;j++) {
            test(i,j,f16_add2(i,j));
            test(i | 0x8000,j,f16_add2(i | 0x8000,j));
            test(i | 0x8000,j | 0x8000,f16_add2(i | 0x8000,j | 0x8000));
            test(i,j | 0x8000,f16_add2(i,j | 0x8000));
        }
    }
    printf("Full DOne\n");*/
    for(i=0;i<500;i++) {
        if(!test_op(i))
            return 1;
    }
    printf("Done!");
    return 0;
}
