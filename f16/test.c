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
    if(rx != r) {
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

static int fix_sign(int r)
{
    if(r<0) {
        r=-r | 0x8000;
    }
    return r;
}

extern short f16_add2(short,short);

int main()
{
    int i,j,v,vref;
    //test(58002,25182,52864);
    //return 0;
    printf("Int  \n");
    for(i=-15;i<15;i++) {
        for(j=-15;j<15;j++) {
            v=f16_add(f16_from_int(i),f16_from_int(j));
            vref = f16_from_int(i+j);
            if(v!=vref) {
                printf("%d + %d -> %x != %x\n",i,j,v,vref);
                return 1;
            }
        }
    }
    printf("Subnormals\n");
    for(i=-2047;i<2048;) {
        for(j=-2047;j<2048;) {
            short si=fix_sign(i),sj=fix_sign(j);
            if(-2048 < i+j && i+j < 2048 && (vref=fix_sign(i+j)) != (v=f16_add(si,sj))) {
                printf("%d+%d=%d \n",i,j,i+j);
                printf("%x+%x=%x!=%x fail\n",i,j,vref,v);
                printf("old = %x",f16_add2(si,sj));
                return 1;
            }
            if(-2048 < i-j && i - j < 2048 && (vref=fix_sign(i-j)) != (v=f16_sub(si,sj))) {
                printf("%d-%d=%d \n",i,j,i-j);
                printf("%x-%x=%x!=%x fail %\n",i,j,vref,v);
                return 1;
            }
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
    printf("Start Reg\n");
    for(i=0;i<500;i++) {
        if(!test_op(i))
            return 1;
    }
    printf("Done!");
    return 0;
}
