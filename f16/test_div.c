#include <stdio.h>
#include <stdlib.h>

extern unsigned long div_24_by_15(unsigned long x,unsigned short y);

int main()
{
    for(int i=0;i<1000;i++) {
        unsigned short d;
        unsigned long v;
        switch(i) {
        case 0: v=0xFFFFFFul; d=0x7FFF; break;
        case 1: v=0xFFFFFFul; d=1; break;
        case 2: v=0; d=0x7FFF; break;
        case 3: v=0; d=1; break;
        default:
            v=rand();
            v = (v<<8) + rand();
            v=v & 0xFFFFFFul;
            d=0x7FFF&(rand() + rand());
            if(d==0)
                d++;
        }
        unsigned long ref = v/d;
        unsigned long res = div_24_by_15(v,d);
        if(ref != res) {
            printf("test %d: %lu / %u = %lu\n GOT %lu    \n",i,v,d,ref,res);
            return 1;
        }
    }
    printf("Ok\n");
    return 0;
}
