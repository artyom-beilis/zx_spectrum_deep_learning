#ifdef __linux
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "train_samples.h"
#include "test_samples.h"
#else
const int train_samples_size=64;
#endif

#define BATCH_SIZE 10
#define INPSIZE 8
#define KERNELS 8
#define KSIZE   3
#define INTERM_SIZE (INPSIZE - KSIZE + 1)
#define POOL_SIZE (INTERM_SIZE  >> 1)
#define FLAT_SIZE (POOL_SIZE*POOL_SIZE*KERNELS)
#define CLASS_NO 10


inline float max(float a,float b)
{
    return a > b ? a : b;
}

inline float relu(float x)
{
    if( x > 0.0f)
        return x;
    return 0.0f;
}

typedef struct Params {
    float conv_kernel[KERNELS][KSIZE][KSIZE];
    float conv_offset[KERNELS];
    float ip_mat[CLASS_NO][FLAT_SIZE];
    float ip_offset[CLASS_NO];
} Params;

typedef struct Layers {
    float conv_res[KERNELS][INTERM_SIZE][INTERM_SIZE];
    float pool_res[FLAT_SIZE];
    float probs[CLASS_NO];
} Layers;

typedef struct AllData {
    Params params;
    Params params_diffs;
    Layers blobs;
    Layers blob_diffs;
} AllData;


void conv_forward(unsigned char *digit,float kernel[KERNELS][KSIZE][KSIZE],float offset[KERNELS],float top[KERNELS][INTERM_SIZE][INTERM_SIZE])
{
    int n,i,j,r,c;
    unsigned char row;
    float sum;
    for(r=0;r<INTERM_SIZE;r++) {
        for(c=0;c<INTERM_SIZE;c++) {
            for(n=0;n<KERNELS;n++) {
                sum = offset[n];
                for(i=0;i<KSIZE;i++) {
                    row = digit[r+i];
                    for(j=0;j<KSIZE;j++) {
                        if((row >> (c+j)) & 1) {
                            sum+= kernel[n][i][j];
                        }
                    }
                }
                top[n][r][c] = sum;
            }
        }
    }
}

void conv_backward(unsigned char *digit,float kernel[KERNELS][KSIZE][KSIZE],  float offset[KERNELS],  float top[KERNELS][INTERM_SIZE][INTERM_SIZE],
                                        float kernel_d[KERNELS][KSIZE][KSIZE],float offset_d[KERNELS],float top_d[KERNELS][INTERM_SIZE][INTERM_SIZE])
{
    int n,i,j,r,c;
    unsigned char row;
    for(n=0;n<KERNELS;n++) {
        float sum=0.0f;
        for(i=0;i<INTERM_SIZE;i++) {
            for(j=0;j<INTERM_SIZE;j++) {
                sum+=top_d[n][i][j];
            }
        }
        offset_d[n] += sum;
    }
    for(r=0;r<INTERM_SIZE;r++) {
        for(c=0;c<INTERM_SIZE;c++) {
            for(n=0;n<KERNELS;n++) {
                for(i=0;i<KSIZE;i++) {
                    row = digit[r+i];
                    for(j=0;j<KSIZE;j++) {
                        if((row >> (c+j)) & 1) {
                            kernel_d[n][i][j] += top_d[n][r][c];
                        }
                    }
                }
            }
        }
    }
}


static unsigned char pooling_selection_mask[FLAT_SIZE];

void max_pool_2x2_relu_forward(float bottom[KERNELS][INTERM_SIZE][INTERM_SIZE],float top[FLAT_SIZE])
{
    int n,r,c,r2,c2,pos,index;
    float m,tmp;
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


void max_pool_2x2_relu_backward(float bottom[KERNELS][INTERM_SIZE][INTERM_SIZE],float top[FLAT_SIZE],
                                float bottom_d[KERNELS][INTERM_SIZE][INTERM_SIZE],float top_d[FLAT_SIZE])
{
    int k,r,c,index,dr,dc;
    int pos=0;
    for(k=0;k<FLAT_SIZE;k++) {
        if(top[k] <= 0.0f)
            top_d[k] = 0.0;
    }
    for(k=0;k<KERNELS;k++) {
        for(r=0;r<POOL_SIZE;r++) {
            for(c=0;c<POOL_SIZE;c++) {
                index = pooling_selection_mask[pos];
                for(dr=0;dr<2;dr++)
                    for(dc=0;dc<2;dc++)
                        bottom_d[k][r*2+dr][c*2+dc]=(dr == (index >> 1) && dc == (index & 1)) ? top_d[pos] : 0.0f;
                pos++;
            }
        }
    }
}

void ip_forward(float bottom[FLAT_SIZE],float top[CLASS_NO],float offset[CLASS_NO],float M[CLASS_NO][FLAT_SIZE])
{
    float sum;
    int i,j;
    for(i=0;i<CLASS_NO;i++) {
        sum = offset[i];
        for(j=0;j<FLAT_SIZE;j++) {
            sum += bottom[j] * M[i][j];
        }
        top[i]=sum;
    }
}

void ip_backward(float bottom  [FLAT_SIZE],float   top[CLASS_NO],float   offset[CLASS_NO],float   M[CLASS_NO][FLAT_SIZE],
                 float bottom_d[FLAT_SIZE],float top_d[CLASS_NO],float offset_d[CLASS_NO],float M_d[CLASS_NO][FLAT_SIZE])
{
    int i,j,k;
    for(k=0;k<CLASS_NO;k++)
        offset_d[k] += top_d[k];
    for(j=0;j<FLAT_SIZE;j++) 
        bottom_d[j] = 0.0f;
    for(i=0;i<CLASS_NO;i++) {
        for(j=0;j<FLAT_SIZE;j++) {
            M_d[i][j] += bottom[j] * top_d[i];
            bottom_d[j] += M[i][j] * top_d[i];
        }
    }
}

int euclidean_loss_forward(float bottom[CLASS_NO],float *loss,int label)
{
    int i,max_index;
    float sum=0.0f,target,max_val;
    float sdiff;
    const float factor = 0.5;

    max_index = 0;
    max_val = bottom[0];
    for(i=1;i<CLASS_NO;i++) {
        if(bottom[i] > max_val) {
            max_index=i;
            max_val = bottom[i];
        }
    }
    for(i=0;i<CLASS_NO;i++) {
        target = label == i ? 1.0f : 0.0f;
        sdiff = target - bottom[i];
        sum += sdiff * sdiff;
    }
    *loss += factor * sum;
    return max_index == label;
}

void euclidean_loss_backward(float bottom[CLASS_NO],float diff[CLASS_NO],int label)
{
    int i;
    float target;
    float sdiff;

    for(i=0;i<CLASS_NO;i++) {
        target = label == i ? 1.0f : 0.0f;
        sdiff = bottom[i] - target;
        diff[i] = sdiff;
    }
}

int net_forward(unsigned char *digit,int label,Params *p,Layers *l,float *loss)
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

float forward_backward(AllData *d,unsigned char *digit,int label,float *loss)
{
    float r;
    r = net_forward(digit,label,&d->params,&d->blobs,loss);
    net_backward(digit,label,&d->params,&d->params_diffs,&d->blobs,&d->blob_diffs);
    return r;
}


void apply_update(Params *params,Params *params_diff,float lr,float inv_lr,float wd,float momentum)
{
    const int size = sizeof(Params) / sizeof(float);
    float *p=(float*)(params);
    float *pd=(float*)(params_diff);
    int i;
    for(i=0;i<size;i++) {
        float v = pd[i] * lr; 
        p[i] = p[i] - v - p[i] * wd;
        pd[i] = v * inv_lr * momentum;
    }
}

unsigned short randv()
{
    static unsigned short lfsr = 0xACE1u;
    unsigned short bit;
    bit = ((lfsr >> 0) ^ (lfsr >> 2) ^ (lfsr >> 3) ^ (lfsr >> 5));
    lfsr = (lfsr >> 1) | (bit << 15);
    return lfsr;
}

float gauus()
{
    unsigned res = 0;
    int i;
    static float const factor = (16.0 / 65536.0);
    for(i=0;i<12;i++)
        res += randv() >> 4;
    return factor * res - 6.0f;

}

void xavier(float *v,int size,int Nin,int Nout)
{
    int i;
    float factor = 2.0 / (Nin + Nout);
    for(i=0;i<size;i++) {
        v[i] = factor * gauus();
    }
}

void init_params(Params *p)
{
    int i;
    xavier(&p->ip_mat[0][0],CLASS_NO*FLAT_SIZE,FLAT_SIZE,CLASS_NO);
    for(i=0;i<CLASS_NO;i++)
        p->ip_offset[i]=0.0f;

    xavier(&p->conv_kernel[0][0][0],KERNELS*KSIZE*KSIZE,KERNELS*KSIZE*KSIZE,KERNELS*KSIZE*KSIZE);
    for(i=0;i<KERNELS;i++)
        p->conv_offset[i]=0.0f;
}

AllData data;

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

#define BASE_LR 0.01

unsigned char sample[8];
float blr = BASE_LR;
float inv_blr = (1.0/BASE_LR);
void train(int epoch)
{
    if(epoch == 0) {
        init_params(&data.params);
    }
#ifdef __linux
    printf("Epoch %d... ",epoch);
#endif
    float acc = 0.0;
    for(int sample_id=0;sample_id < train_samples_size;sample_id++) {
        float loss = 0.0;
        for(int i=0;i<10;i++) {
            get_character(sample,i*rows_for_digit + sample_id / 32,sample_id % 32);
            mark_character(i,sample_id,ST_TRAIN);
            float cur_ac = forward_backward(&data,sample,i,&loss);
            if(cur_ac == 0.0f)
                mark_character(i,sample_id,ST_FAIL);
            else
                mark_character(i,sample_id,ST_OK);
            acc += cur_ac;
        }
        if(epoch==2 && sample_id == 0) {
            blr*=0.1;
            inv_blr*=10.0;
        }
        apply_update(&data.params,&data.params_diffs,blr,inv_blr,0.0005,0.9);
    }
#ifdef __linux
    printf("Accuracy %f%%\n",acc / train_samples_size *10);
#endif
}
void test()
{
    int N=0;
    float acc = 0.0;
    for(int b=0;b<train_samples_size;b++) {
        float loss = 0.0;
        for(int i=0;i<10;i++) {
            int sample_id = b;
            get_character(sample,i*rows_for_digit + sample_id / 32,sample_id % 32);
            mark_character(i,sample_id,ST_TRAIN);
            float cur_ac = net_forward(sample,i,&data.params,&data.blobs,&loss);
            if(cur_ac == 0.0f)
                mark_character(i,sample_id,ST_FAIL);
            else
                mark_character(i,sample_id,ST_OK);
            acc += cur_ac;
            N++;
        }
    }
#ifdef __linux
    printf("Test Accuracy %f%%\n",acc/N*100);
#endif
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

void make_screen(unsigned char samples[10][sizeof(train_samples) / 10 / 8][8],char const *name)
{
    memset(screen,0,6144);
    memset(screen+6144,56,32*24);

    for(int b=0;b<train_samples_size;b++) {
        for(int c=0;c<10;c++) {
            print_character(samples[c][b],c*rows_for_digit + b / 32,b % 32);
        }
    }
    FILE *f=fopen(name,"w");
    fwrite(screen,sizeof(screen),1,f);
    fclose(f);

}
#endif

#ifdef __linux
int main()
{
    printf("Data Size = %d\n",(int)sizeof(AllData));
    make_screen(train_samples,"screen.scr");
    for(int e=0;e<5;e++)
        train(e);
    make_screen(test_samples,"test_screen.scr");
    test();
    return 0;
}
#else
int main()
{
    unsigned char *statep = (void*)(25599);
    int epoch = *statep;
    if(epoch < 255) {
        train(epoch);
    }
    else {
        test();
    }
    return 0;
}
#endif

