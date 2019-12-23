#include "fixed12_math.h"
#include <limits.h>
#include <stdio.h>

unsigned clock()
{
    unsigned *ptr = (void*)(23672);
    return *ptr;
}

int fixed12_mpl_ref(int a,int b)
{
    int neg = (a < 0) ^ (b < 0);
    if(a < 0)
        a=-a;
    if(b < 0)
        b=-b;
    long res = (long)(a) * b;
    if(neg)
        res = -res;
    res += 2048;
    return res >> 12;
}

int test_eq(int a,int b)
{
    long v1 = mpl_2int_to_long(a,b);
    long v2 = ((long)a * b);
    if(v1 != v2) {
        printf("I: %d * %d  new=%ld std=%ld\n",a,b,v1,v2);
        return 1;
    }

    int f1 = fixed12_mpl(a,b);
    int f2 = fixed12_mpl_ref(a,b);
    if(f1 != f2) {
        printf("F: %d * %d  new=%d std=%d\n",a,b,f1,f2);
        return 1;
    }

    return 0;
}

int main()
{
    int errors = 0;
    for(int x=0;x < 30000; x=x*2 + 3) {
        for(int y=0;y < 30000; y=y*2 + 3) {
            errors += test_eq(x,y);
            errors += test_eq(x,-y);
            errors += test_eq(-x,y);
            errors += test_eq(-x,-y);
            if(errors > 20)
                goto stop_it;
        }
    }
    test_eq(INT_MAX,INT_MAX);
    test_eq(INT_MAX,INT_MIN);
    test_eq(INT_MIN,INT_MAX);
    test_eq(INT_MIN,INT_MIN);
stop_it:
    
    __asm
        ei
    __endasm;

    int v1=0,v2=0;
    int N=10000;
    unsigned start,passed;
    printf("Custom         ");
    start = clock();
    for(int i=0;i<N;i++) {
        v1+=fixed12_mpl(4096+i,4096-i);
    }
    passed = clock() - start;
    printf(" %d.%01ds\n", passed / 50, passed / 5);
    printf("Standard       ");
    start = clock();
    for(int i=0;i<N;i++) {
        v2+=(int)(((long)(4096+i) * (4096-i)) >> 12);
    }
    passed = clock() - start;
    printf(" %d.%01ds\n", passed / 50, passed / 5);
    printf("%04x:%04x\n",v1,v2);

    return 0;
}


