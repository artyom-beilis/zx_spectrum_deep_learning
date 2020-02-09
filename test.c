#include <stdint.h>
int oval;
int32_t ival=1234;

short f16_from_int(int32_t v)
{
    return (v+1)>>5;
}


int main()
{
    oval = f16_from_int(ival);
    return 0;
}
