#include "float16.h"
#include "ref.h"
#include <stdio.h>

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
        printf("Test %d: %d %c %d = %d != %d\n",n,ax,op,bx,r,rx);
        return 0;
    }
    return 1;
}

int main()
{
    int i;
    for(i=0;i<500;i++) {
        if(!test_op(i))
            return 1;
    }
    return 0;
}
