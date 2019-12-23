#include <stdio.h>
#include <limits.h>
int main()
{
    for(int i=-32768;i<=32767;i++) {
        short x=i;
        short neg1 = (x ^ 0xFFFF) + 1;
        short neg2 = -x;
        if(neg1!=neg2)
            printf("%d %d!=%dx\n",x,neg1,neg2);
    }
    return 0;
}
