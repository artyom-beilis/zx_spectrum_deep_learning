#ifdef __linux
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "train_samples.h"
#include "test_samples.h"
//#include "half.hpp"
#else
#ifndef __linux
#include "fixed12_math.h"
#endif
#include <stdio.h>
const int train_samples_size=64;
#include "enable_timer.h"
#endif
#include "config.h"

#define INTERM_SIZE (INPSIZE - KSIZE + 1)
#define POOL_SIZE (INTERM_SIZE  >> 1)
#define FLAT_SIZE (POOL_SIZE*POOL_SIZE*KERNELS)


#define FIX_SHIFT 12
#define FIX_SCALE (1 << FIX_SHIFT)
#define FIX_ROUND (1 << (FIX_SHIFT-1))
#define FIX_MASK  (FIX_SCALE - 1)

#ifdef __linux

short fixed12_mpl_ref(short a,short b)
{
    int neg = (a < 0) ^ (b < 0);
    if(a < 0)
        a=-a;
    if(b < 0)
        b=-b;
    int res = (int)(a) * b;
    if(neg)
        res = -res;
    res += FIX_ROUND;
    res >>= 12;
    return res;
}

int fixed12_mpl_no_shift(short a,short b)
{
    int neg = (a < 0) ^ (b < 0);
    if(a < 0)
        a=-a;
    if(b < 0)
        b=-b;
    
    unsigned short au = a;
    unsigned short bu = b;
    unsigned res = (unsigned)au * bu;
    int val = res;
    if(neg)
        val = -val;
    return val;
}

#define real_mpl(a,b) fixed12_mpl_ref(a,b)
#define real_mpl_nshift(a,b) fixed12_mpl_no_shift(a,b)
#else // zx spectrum
#define real_mpl(a,b) fixed12_mpl((a),(b))
#define real_mpl_nshift(a,b) mpl_2int_to_long(a,b)
#endif

#define from_float(a) ((short)((a)*FIX_SCALE))
#define to_float(a) ((float)(a) * ( 1.0f / FIX_SCALE))

typedef short RealType;

#define real_zero 0
#define real_one FIX_SCALE
#define real_half(v) ((v)>>1)

inline RealType max(RealType a,RealType b)
{
    return a > b ? a : b;
}

inline RealType relu(RealType x)
{
    if( x > real_zero)
        return x;
    return real_zero;
}

typedef struct Params {
    RealType conv_kernel[KERNELS][KSIZE][KSIZE];
    RealType conv_offset[KERNELS];
    RealType ip_mat[CLASS_NO][FLAT_SIZE];
    RealType ip_offset[CLASS_NO];
} Params;

typedef struct Layers {
    RealType conv_res[KERNELS][INTERM_SIZE][INTERM_SIZE];
    RealType pool_res[FLAT_SIZE];
    RealType probs[CLASS_NO];
} Layers;

typedef struct AllData {
    Params params;
    Params params_diffs;
    Params params_vel;
    Layers blobs;
    Layers blob_diffs;
} AllData;

AllData data;



void conv_forward(unsigned char *digit,RealType kernel[KERNELS][KSIZE][KSIZE],RealType offset[KERNELS],RealType top[KERNELS][INTERM_SIZE][INTERM_SIZE])
{
    int n,i,j,r,c;
    unsigned char row;
    for(r=0;r<INTERM_SIZE;r++) {
        for(c=0;c<INTERM_SIZE;c++) {
            for(n=0;n<KERNELS;n++)
                top[n][r][c] = offset[n];
            for(i=0;i<KSIZE;i++) {
                row = digit[r+i];
                for(j=0;j<KSIZE;j++) {
                    if((row >> (c+j)) & 1) {
                        for(n=0;n<KERNELS;n++) 
                            top[n][r][c]+= kernel[n][i][j];
                    }
                }
            }
        }
    }
}

void conv_backward(unsigned char *digit,RealType kernel[KERNELS][KSIZE][KSIZE],  RealType offset[KERNELS],  RealType top[KERNELS][INTERM_SIZE][INTERM_SIZE],
                                        RealType kernel_d[KERNELS][KSIZE][KSIZE],RealType offset_d[KERNELS],RealType top_d[KERNELS][INTERM_SIZE][INTERM_SIZE])
{
    int n,i,j,r,c;
    unsigned char row;
    for(n=0;n<KERNELS;n++) {
        RealType sum = real_zero;
        for(i=0;i<INTERM_SIZE;i++) {
            for(j=0;j<INTERM_SIZE;j++) {
                sum+=top_d[n][i][j];
            }
        }
        offset_d[n] += sum;
    }
    for(r=0;r<INTERM_SIZE;r++) {
        for(c=0;c<INTERM_SIZE;c++) {
            for(i=0;i<KSIZE;i++) {
                row = digit[r+i];
                for(j=0;j<KSIZE;j++) {
                    if((row >> (c+j)) & 1) {
                        for(n=0;n<KERNELS;n++) 
                            kernel_d[n][i][j] += top_d[n][r][c];
                    }
                }
            }
        }
    }
}


static unsigned char pooling_selection_mask[FLAT_SIZE];

void max_pool_2x2_relu_forward(RealType bottom[KERNELS][INTERM_SIZE][INTERM_SIZE],RealType top[FLAT_SIZE])
{
    int n,r,c,r2,c2,pos,index;
    RealType m,tmp;
    pos = 0;
    for(n=0;n<KERNELS;n++) {
        for(r=0;r<POOL_SIZE;r++) {
            for(c=0;c<POOL_SIZE;c++) {
                r2=r*2;
                c2=c*2;;
                index=0;
                m   = bottom[n][r2+0][c2+0];
                tmp = bottom[n][r2+0][c2+1];
                if(tmp > m) {
                    index = 1;
                    m = tmp;
                }
                tmp = bottom[n][r2+1][c2+0];
                if(tmp > m) {
                    index = 2;
                    m = tmp;
                }
                tmp = bottom[n][r2+1][c2+1];
                if(tmp > m) {
                    index = 3;
                    m = tmp;
                }
                pooling_selection_mask[pos]=index;
                top[pos++] = relu(m);
            }
        }
    }
}


void max_pool_2x2_relu_backward(RealType bottom[KERNELS][INTERM_SIZE][INTERM_SIZE],RealType top[FLAT_SIZE],
                                RealType bottom_d[KERNELS][INTERM_SIZE][INTERM_SIZE],RealType top_d[FLAT_SIZE])
{
    int k,r,c,index,dr,dc;
    int pos=0;
    for(k=0;k<FLAT_SIZE;k++) {
        if(top[k] <= real_zero)
            top_d[k] = real_zero;
    }
    for(k=0;k<KERNELS;k++) {
        for(r=0;r<POOL_SIZE;r++) {
            for(c=0;c<POOL_SIZE;c++) {
                index = pooling_selection_mask[pos];
                for(dr=0;dr<2;dr++)
                    for(dc=0;dc<2;dc++)
                        bottom_d[k][r*2+dr][c*2+dc]=(dr == (index >> 1) && dc == (index & 1)) ? top_d[pos] : real_zero;
                pos++;
            }
        }
    }
}

void ip_forward(RealType bottom[FLAT_SIZE],RealType top[CLASS_NO],RealType offset[CLASS_NO],RealType M[CLASS_NO][FLAT_SIZE])
{
    RealType sum;
    int i,j;
    for(i=0;i<CLASS_NO;i++) {
        sum = offset[i];
        for(j=0;j<FLAT_SIZE;j++) {
            sum += real_mpl(bottom[j],M[i][j]);
        }
        top[i]=sum;
    }
}

void ip_backward(RealType bottom  [FLAT_SIZE],RealType   top[CLASS_NO],RealType   offset[CLASS_NO],RealType   M[CLASS_NO][FLAT_SIZE],
                 RealType bottom_d[FLAT_SIZE],RealType top_d[CLASS_NO],RealType offset_d[CLASS_NO],RealType M_d[CLASS_NO][FLAT_SIZE])
{
    int i,j,k;
    for(k=0;k<CLASS_NO;k++)
        offset_d[k] += top_d[k];
    for(j=0;j<FLAT_SIZE;j++) 
        bottom_d[j] = real_zero;
    for(i=0;i<CLASS_NO;i++) {
        for(j=0;j<FLAT_SIZE;j++) {
            M_d[i][j] += real_mpl(bottom[j],top_d[i]);
            bottom_d[j] += real_mpl(M[i][j],top_d[i]);
        }
    }
}

int euclidean_loss_forward(RealType bottom[CLASS_NO],RealType *loss,int label)
{
    int i,max_index;
    RealType sum = real_zero,target,max_val;
    RealType sdiff;

    max_index = 0;
    max_val = bottom[0];
    for(i=1;i<CLASS_NO;i++) {
        if(bottom[i] > max_val) {
            max_index=i;
            max_val = bottom[i];
        }
    }
    for(i=0;i<CLASS_NO;i++) {
        target = label == i ? real_one : real_zero;
        sdiff = target - bottom[i];
        sum += real_mpl(sdiff,sdiff);
    }
    *loss += real_half(sum);
    return max_index == label;
}

void euclidean_loss_backward(RealType bottom[CLASS_NO],RealType diff[CLASS_NO],int label)
{
    int i;
    RealType target;
    RealType sdiff;

    for(i=0;i<CLASS_NO;i++) {
        target = label == i ? real_one : real_zero;
        sdiff = bottom[i] - target;
        diff[i] = sdiff;
    }
}

int net_forward(unsigned char *digit,int label,Params *p,Layers *l,RealType *loss)
{
    conv_forward(digit,p->conv_kernel,p->conv_offset,l->conv_res);
    max_pool_2x2_relu_forward(l->conv_res,l->pool_res);
    ip_forward(l->pool_res,l->probs,p->ip_offset,p->ip_mat);
    return euclidean_loss_forward(l->probs,loss,label);
}

void net_backward(unsigned char *digit,int label,Params *p,Params *pd,Layers *l,Layers *ld)
{
    euclidean_loss_backward(l->probs,ld->probs,label);
    ip_backward(l->pool_res,l->probs,p->ip_offset,p->ip_mat,
                ld->pool_res,ld->probs,pd->ip_offset,pd->ip_mat);
    max_pool_2x2_relu_backward(l->conv_res,l->pool_res,ld->conv_res,ld->pool_res);
    conv_backward(digit,p->conv_kernel,p->conv_offset,l->conv_res,
                       pd->conv_kernel,pd->conv_offset,ld->conv_res);
}

int forward_backward(AllData *d,unsigned char *digit,int label,RealType *loss)
{
    int r = net_forward(digit,label,&d->params,&d->blobs,loss);
    net_backward(digit,label,&d->params,&d->params_diffs,&d->blobs,&d->blob_diffs);
    return r;
}


#if 0 
void apply_update_vfloat(float lr,float wd,float momentum)
{
    const int size = sizeof(Params) / sizeof(RealType);
    RealType *p=(RealType*)(&data.params);
    RealType *pd=(RealType*)(&data.params_diffs);
    RealType *v=(RealType*)(&data.params_vel);
    int i;
    float wd_fact = 1.0 - wd;
    for(i=0;i<size;i++) {
        float vel = to_float(v[i]) * momentum + lr * to_float(pd[i]);
        p[i] = from_float(to_float(p[i]) * wd_fact  - vel);
        v[i]= from_float(vel);
        pd[i] = real_zero;
    }
}
void apply_update_fixed(RealType lr,RealType wd,RealType momentum)
{
    apply_update_vfloat(to_float(lr),to_float(wd),to_float(momentum));
}
#else

void apply_update_fixed(RealType lr,RealType wd,RealType momentum)
{
    const int size = sizeof(Params) / sizeof(RealType);
    RealType *p=(RealType*)(&data.params);
    RealType *pd=(RealType*)(&data.params_diffs);
    RealType *v=(RealType*)(&data.params_vel);
    int i;
    RealType wd_fact = real_one - wd;
    for(i=0;i<size;i++) {
        int32_t vel = real_mpl_nshift(v[i],momentum) + real_mpl_nshift(lr,pd[i]);
        p[i] = (real_mpl_nshift(p[i], wd_fact)  - vel + FIX_ROUND) >> FIX_SHIFT;
        v[i]= (vel + FIX_ROUND) >> FIX_SHIFT;
        pd[i] = real_zero;
    }
}
#endif

unsigned short randv()
{
    static unsigned short lfsr = 0xACE1u;
    unsigned short bit;
    bit = ((lfsr >> 0) ^ (lfsr >> 2) ^ (lfsr >> 3) ^ (lfsr >> 5));
    lfsr = (lfsr >> 1) | (bit << 15);
    return lfsr;
}

#if 0
float gauus()
{
    unsigned res = 0;
    int i;
    static float const factor = 16.0f / 65536.0;
    for(i=0;i<12;i++)
        res += randv() >> 4;
    return factor * res - 6.0f;

}

void xavier(RealType *v,int size,int Nin,int Nout)
{
    int i;
    float factor = 2.0 / (Nin + Nout);
    for(i=0;i<size;i++) {
        v[i] = from_float(factor * gauus());
    }
}
#else

RealType gauus(RealType sigma)
{
#if FIX_SHIFT != 12
#error "Does not work with other shift!"
#endif
    unsigned res = 0;
    int i;
    for(i=0;i<12;i++)
        res += randv() >> 4;
    return real_mpl(res - 6 * FIX_SCALE,sigma);
}
void xavier(RealType *v,int size,int Nin,int Nout)
{
    int i;
    RealType sigma = FIX_SCALE * 2 / (Nin + Nout);
    unsigned short cs=0;
    for(i=0;i<size;i++) {
        v[i] = gauus(sigma);
        cs = cs ^ v[i];
    }
}
#endif

void init_params(Params *p)
{
    int i;
    xavier(&p->ip_mat[0][0],CLASS_NO*FLAT_SIZE,FLAT_SIZE,CLASS_NO);
    for(i=0;i<CLASS_NO;i++)
        p->ip_offset[i]=real_zero;

    xavier(&p->conv_kernel[0][0][0],KERNELS*KSIZE*KSIZE,KERNELS*KSIZE*KSIZE,KERNELS*KSIZE*KSIZE);
    for(i=0;i<KERNELS;i++)
        p->conv_offset[i]=real_zero;
}


#ifdef __linux
unsigned char screen[6144 + 32*24];
#else
unsigned char *screen = (void *)(16384);
#endif

const int rows_for_digit = 2;

#define ST_TRAIN    0
#define ST_OK       1
#define ST_FAIL     2

void get_character(unsigned char *chr,int r,int c)
{
    unsigned char *tgt = screen;
    tgt += (r % 8)*32+c + (32*8*8) * (r / 8);
    for(int k=0;k<8;k++) {
        *chr++ = *tgt;
        tgt += 256;
    }
}

void mark_character(int digit,int batch,int status)
{
    int addr = digit * train_samples_size + batch;
    unsigned char *mark = screen + 6144;
    if(addr > 32*25) {
        return;
    }
    mark += addr;
    switch(status) {
    case ST_TRAIN:  *mark = (3) << 3; break;
    case ST_OK:     *mark = (2) << 4; break;
    case ST_FAIL:   *mark = (1) << 4; break;
    }
}

unsigned char sample[8];
RealType blr = FIX_SCALE / 100; // 0.01f


RealType train(int epoch)
{
    if(epoch == 0) {
        init_params(&data.params);
    }
    int acc = 0;
    for(int sample_id=0;sample_id < DATA_SIZE;sample_id++) {
        RealType loss=real_zero;
        for(int i=0;i<CLASS_NO;i++) {
            get_character(sample,i*rows_for_digit + sample_id / 32,sample_id % 32);
            mark_character(i,sample_id,ST_TRAIN);
            int cur_ac = forward_backward(&data,sample,i,&loss);
            if(cur_ac == 0)
                mark_character(i,sample_id,ST_FAIL);
            else
                mark_character(i,sample_id,ST_OK);
            acc += cur_ac;
        }
        if(epoch==2 && sample_id == 0) {
            blr /= 10;
        }
        if(sample_id % ITER_SIZE == (ITER_SIZE-1)) {
            // momentum = 0.9, wd = 0.0005
            apply_update_fixed(blr,(short)(FIX_SCALE * 5l / 10000),(short)(FIX_SCALE * 9l / 10));
        }
    }
    return ((int32_t)acc * FIX_SCALE / (train_samples_size * CLASS_NO)); 
}
RealType test()
{
    int N=0;
    int acc=0;
    for(int b=0;b<DATA_SIZE;b++) {
        RealType loss=real_zero;
        for(int i=0;i<CLASS_NO;i++) {
            int sample_id = b;
            get_character(sample,i*rows_for_digit + sample_id / 32,sample_id % 32);
            mark_character(i,sample_id,ST_TRAIN);
            int cur_ac = net_forward(sample,i,&data.params,&data.blobs,&loss);
            if(cur_ac == 0)
                mark_character(i,sample_id,ST_FAIL);
            else
                mark_character(i,sample_id,ST_OK);
            acc += cur_ac;
            N++;
        }
    }
    return ((int32_t)acc * FIX_SCALE / (train_samples_size * CLASS_NO)); 
}

#ifdef __linux
void print_character(unsigned char *chr,int r,int c)
{
    unsigned char *tgt = screen;
    tgt += (r % 8)*32+c + (32*8*8) * (r / 8);
    for(int k=0;k<8;k++) {
        *tgt = *chr++;
        tgt += 256;
    }
}

void make_screen(unsigned char samples[10][sizeof(train_samples) / 10 / 8][8])
{
    memset(screen,0,6144);
    memset(screen+6144,56,32*24);

    for(int b=0;b<train_samples_size;b++) {
        for(int c=0;c<10;c++) {
            print_character(samples[c][b],c*rows_for_digit + b / 32,b % 32);
        }
    }
}
#endif

#ifdef __linux
int main()
{
    printf("Data Size = %d float_size=%d\n",(int)sizeof(AllData),(int)sizeof(RealType));
    make_screen(train_samples);
    for(int e=0;e<EPOCHS;e++) {
        RealType acc = train(e);
        printf("Epoch=%d, accuracy = %3.1f%%\n",e,100*to_float(acc));
    }
    make_screen(test_samples);
    RealType acc = test();
    printf("Test accuracy = %3.1f%%\n",100*to_float(acc));
    return 0;
}
#else
int main()
{
    enable_timer();
    unsigned char *statep = (void*)(25599);
    int epoch = *statep;
    RealType acc;
    if(epoch < 255) {
        acc = train(epoch);
    }
    else {
        acc = test();
    }
    return acc;
}
#endif

