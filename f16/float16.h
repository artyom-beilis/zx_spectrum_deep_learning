#ifndef FLOAT16_H
#define FLOAT16_H
typedef struct Float16 {
    union {
        unsigned short value;
        struct {
            unsigned short fraction : 10;
            unsigned short exponent : 5;
            unsigned short sign : 1;
        };
    } m;
} float16_t;

#define f16_one 15360
#define f16_half 14336
short f16_add(short a,short b);
short f16_sub(short a,short b);
short f16_mul(short a,short b);
short f16_div(short a,short b);
short f16_neg(short a);
short f16_from_int(short v);
short f16_int(short v);

int f16_gte(short a,short b);
int f16_gt(short a,short b);
int f16_eq(short a,short b);
int f16_lte(short a,short b);
int f16_lt(short a,short b);
int f16_neq(short a,short b);

#endif
