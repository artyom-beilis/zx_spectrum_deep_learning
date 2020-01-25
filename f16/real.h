#ifndef REAL_H
#define REAL_H
#ifdef USE_F16
#include "float16.h"
typedef short real_t;
#define real_gt(a,b) f16_gt((a),(b))
#define real_div(a,b) f16_div((a),(b))
#define real_mul(a,b) f16_mul((a),(b))
#define real_add(a,b) f16_add((a),(b))
#define real_sub(a,b) f16_sub((a),(b))
#define real_one    f16_one
#define real_from_int(a) f16_from_int((a))
#define real_int(a) f16_int((a))
#define real_neg(a) f16_neg((a))
#define real_name "float16"
#elif defined USE_FIXED
typedef short real_t;
#define real_add(a,b) ((a)+(b))
#define real_sub(a,b) ((a)-(b))
#define real_neg(a) (-(a))
#define real_gt(a,b) ((a) > (b))

#ifdef USE_FIXED88
#include "fixed8_math.h"
#define real_div(a,b) ((short)(((long)(a)<<8)/b))
#define real_mul(a,b) fixed8_mpl((a),(b))
#define real_one 256
#define real_from_int(a) ((a)<<8)
#define real_int(a) ((a)>>8)
#define real_name "fixed 8:8"
#else
#include "fixed12_math.h"
#define real_div(a,b) ((short)((long)((a)<<12)/b))
#define real_mul(a,b) fixed12_mpl((a),(b))
#define real_one 4096
#define real_from_int(a) ((a)<<12)
#define real_int(a) ((a)>>12)
#define real_name "fixed 4:12"
#endif

#elif defined USE_FLOAT
typedef float real_t;
#define real_div(a,b) ((a)/(b))
#define real_mul(a,b) ((a)*(b))
#define real_add(a,b) ((a)+(b))
#define real_sub(a,b) ((a)-(b))
#define real_gt(a,b) ((a) > (b))
#define real_one 1.0f
#define real_from_int(a) ((float)(a))
#define real_int(a) ((int)(a)
#define real_neg(a) (-(a))
#define real_name "float32"
#endif

#endif
