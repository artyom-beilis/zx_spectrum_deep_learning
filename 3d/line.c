#include <stdio.h>
#include <stdlib.h>
#include "enable_timer.h"
#include "real.h"
#include "sph.h"
extern unsigned long  get_addr_and_mask(int r,int c);
extern unsigned long  inc_x_addr_and_mask(unsigned long addr_and_mask) __z88dk_fastcall;
extern unsigned long  dec_x_addr_and_mask(unsigned long addr_and_mask) __z88dk_fastcall;
extern unsigned long  inc_y_addr_and_mask(unsigned long addr_and_mask) __z88dk_fastcall;
extern int draw_line(int x0,int x1,int y0,int y1);
extern int plot(int r,int c);
extern void clear_screen();
extern void show_screen();


void draw2(int x0,int x1,int y0,int y1)
{
    int dx=x1-x0;
    int dy=y1-y0;
    char dx2=dx/2;
    char dy2=dy/2;
    signed char D=dy-dx/2;
    int y=y0;
    int x=x0;
    while(x<=x1) {
        plot(y,x);
        if(D > 0) {
            y++;
            D=D-dx2;
        }
        D=D+dy2;
        x++;
    }
}

void from_center(int dy,int dx)
{
    draw_line(127,127+dx,95,95+dy);
}

unsigned char points[VERTS][2];
unsigned char edge_marks[EDGES];

real_t sin_deg(int a)
{
    unsigned sign=0;
    if(a < 0) {
        a=-a;
        sign = 1;
    }
    a=a%360;
    if(a>=180) {
        a-=180;
        sign ^= 1;
    }
    if(a>=90)
        a=180-a;
    real_t val = sin_deg_a_div_5[a/5];
    if(sign)
        return real_neg(val);
    else
        return val;
}
real_t cos_deg(int a)
{
    return sin_deg(a+90);
}

typedef struct Mat {
    real_t a[3][3];
} Mat;

void mul_M_b(Mat *p,real_t b[3],real_t r[3],real_t alpha,int N)
{
    int i;
    for(i=0;i<N;i++) {
        r[i]=real_mul(p->a[i][0],b[0]);
        r[i]=real_add(r[i],real_mul(p->a[i][1],b[1]));
        r[i]=real_add(r[i],real_mul(p->a[i][2],b[2]));
        if(alpha!=real_one)
            r[i]=real_mul(alpha,r[i]);
    }
}

void make_points(int a,int b)
{
#ifdef USE_F16    
    short factor=real_from_int(25);
    #define conv_to_screen(a) real_int(a)
#elif defined USE_FIXED
    short const factor=25<<7;
    #define conv_to_screen(a) ((a)>>7) 
#elif defined USE_FLOAT
    float factor = 25.0f;
    #define conv_to_screen(a) ((int)(a))
#endif        
    
    real_t sa = sin_deg(a);
    real_t ca = cos_deg(a);
    real_t msa = real_neg(sa);

    real_t sb = sin_deg(b);
    real_t cb = cos_deg(b);
    real_t msb = real_neg(sb);
    int i;

    static Mat m1,m2,m3;
    m1.a[0][0]=ca;   m1.a[0][1]=sa; m1.a[0][2] = 0;
    m1.a[1][0]=msa;  m1.a[1][1]=ca; m1.a[1][2] = 0;
    m1.a[2][0]=0;    m1.a[2][1]=0;  m1.a[2][2] = real_one;

    m2.a[0][0]=real_one; m2.a[0][1]=0;   m2.a[0][2] = 0;
    m2.a[1][0]=0;       m2.a[1][1]=cb;  m2.a[1][2] = sb;
    m2.a[2][0]=0;       m2.a[2][1]=msb; m2.a[2][2] = cb;

    for(i=0;i<3;i++)
        mul_M_b(&m1,m2.a[i],m3.a[i],factor,3);

    real_t x,y,sx,sy,sz;
    for(int i=0;i<VERTS;i++) {
        sx=vertices[i][0];
        sy=vertices[i][1];
        sz=vertices[i][2];

        x=           real_mul(m3.a[0][0],sx);
        x=real_add(x,real_mul(m3.a[0][1],sy));
        x=real_add(x,real_mul(m3.a[0][2],sz));


        y=           real_mul(m3.a[1][0],sx);
        y=real_add(y,real_mul(m3.a[1][1],sy));
        y=real_add(y,real_mul(m3.a[1][2],sz));
        
        points[i][0]=conv_to_screen(x) + 127;
        points[i][1]=conv_to_screen(y) + 64;
    }
}

void mark_visible()
{
    int i;
    for(i=0;i<EDGES;i++)
        edge_marks[i]=0;
    for(i=0;i<SURFS;i++) {
        int p0 = surfs[i][0][0];
        int p1 = surfs[i][0][1];
        int p2 = surfs[i][0][2];
        int x0 = points[p0][0], y0 = points[p0][1];
        int x1 = points[p1][0], y1 = points[p1][1];
        int x2 = points[p2][0], y2 = points[p2][1];

        int dx1 = x2-x1;
        int dx2 = x1-x0;
        int dy1 = y2-y1;
        int dy2 = y1-y0;

        if(dx1*dy2-dx2*dy1 >= 0) {
            edge_marks[surfs[i][1][0]]=1;
            edge_marks[surfs[i][1][1]]=1;
            edge_marks[surfs[i][1][2]]=1;
        }
    }
}

void plot_sph(int dx)
{
    for(int i=0;i<EDGES;i++) {
        if(!edge_marks[i])
            continue;
        int p1 = edges[i][0];
        int p2 = edges[i][1];
        draw_line(points[p1][0]+dx,points[p2][0]+dx,points[p1][1],points[p2][1]);
    }
}
/*
#include <math.h>
void test_star()
{
    static int as=0;
    for(int a=(as+=5);a<360;a+=30) {
        float ang = a*3.14159f/180;
        int dx = 20*sin(ang);
        int dy = 20*cos(ang);
        from_center(dx,dy);
    }
}
*/


int main()
{
    enable_timer();
    start_timer();
    int frames=0;
    make_points(10,20);
    mark_visible();
    for(int j=0;j<250;j+=5) {
        make_points(j,j*2);
        mark_visible();
        clear_screen();
        plot_sph(j/2-45);
        show_screen();
        frames++;
    }
    long total = stop_timer();
    int sec = total / 50;
    int msec = total % 50 * 2;
    int fps = frames * 50 * 10 / total;
    printf("%s: %d frames for %d.%02ds \n",real_name,frames,sec,msec);
    printf("FPS=%d.%d",fps/10,fps%10);
    return 0;
}
