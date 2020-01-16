#include "float16.h"
#include "float16_impl.i"

short f16_add(short a,short b)
{
    float16_t fa,fb;
    fa.m.value = a;
    fb.m.value = b;
    return float16_add(fa,fb).m.value;
}
short f16_sub(short a,short b)
{
    float16_t fa,fb;
    fa.m.value = a;
    fb.m.value = b;
    return float16_sub(fa,fb).m.value;
}
short f16_mul(short a,short b)
{
    float16_t fa,fb;
    fa.m.value = a;
    fb.m.value = b;
    return float16_mul(fa,fb).m.value;
}
short f16_div(short a,short b)
{
    float16_t fa,fb;
    fa.m.value = a;
    fb.m.value = b;
    return float16_div(fa,fb).m.value;
}
short f16_neg(short v)
{
    float16_t fv;
    fv.m.value = v;
    return float16_neg(fv).m.value;
}
short f16_from_int(short v)
{
    return int_to_float16(v).m.value;
}
short f16_int(short v)
{
    float16_t fv;
    fv.m.value = v;
    return float16_int(fv);
}

int f16_gte(short a,short b)
{
    float16_t fa,fb;
    fa.m.value = a;
    fb.m.value = b;
    return float16_gte(fa,fb);
}

int f16_gt(short a,short b)
{
    float16_t fa,fb;
    fa.m.value = a;
    fb.m.value = b;
    return float16_gt(fa,fb);
}
int f16_eq(short a,short b)
{
    float16_t fa,fb;
    fa.m.value = a;
    fb.m.value = b;
    return float16_eq(fa,fb);
}

int f16_lte(short a,short b)
{
    return f16_gte(b,a);
}

int f16_lt(short a,short b)
{
    return f16_gt(b,a);
}
int f16_neq(short a,short b)
{
    return !f16_eq(a,b);
}

