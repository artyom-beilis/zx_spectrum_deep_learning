#include <stdio.h>
#include <stdlib.h>

extern unsigned long  get_addr_and_mask(int r,int c);
extern unsigned long  inc_x_addr_and_mask(unsigned long addr_and_mask) __z88dk_fastcall;
extern unsigned long  dec_x_addr_and_mask(unsigned long addr_and_mask) __z88dk_fastcall;
extern unsigned long  inc_y_addr_and_mask(unsigned long addr_and_mask) __z88dk_fastcall;

int plot(int r,int c)
{
    if(r < 0 || r>=192)
        return 1;
    if(c < 0 || c>=256)
        return 1;
    unsigned long res = get_addr_and_mask(r,c);
    *(unsigned char *)((unsigned)(res)) |= (unsigned char)(res>>16);
    return 0;
}

int xplot(int r,int c)
{
    if(r < 0 || r>=192)
        return 1;
    if(c < 0 || c>=256)
        return 1;
    unsigned long res = get_addr_and_mask(r,c);
    *(unsigned char *)((unsigned)(res)) ^= (unsigned char)(res>>16);
    return 0;
}

void draw3(int x0,int x1,int y0,int y1)
{
    int dx=x1-x0;
    int dy=y1-y0;
    int D=dy-dx/2;
    int y=y0;
    int x=x0;
    while(x<=x1) {
        xplot(y,x);
        if(D > 0) {
            y++;

            D=D-dx;
        }
        D=D+dy;
        x++;
    }
}


void draw2(int x0,int x1,int y0,int y1)
{
    int dx=x1-x0;
    int dy=y1-y0;
    int D=2*dy-dx;
    int y=y0;
    int x=x0;
    while(x<=x1) {
        xplot(y,x);
        if(D > 0) {
            y++;

            D=D-2*dx;
        }
        D=D+2*dy;
        x++;
    }
}

void test_shift()
{
    for(int i=0;i<1000+192;i++) {
        int r=rand() % 256;
        int c=rand() % 192;
        if(i>=1000) {
            r=i-1000;
            c=i-1000;
        }
        long src = get_addr_and_mask(r,c);
        if(r + 1 < 192) {
            long ref = get_addr_and_mask(r+1,c);
            long tgt = inc_y_addr_and_mask(src);
            if(ref != tgt) {
                printf("from %lx -> %lx != %lx for %d+1,%d\n",src,ref,tgt,r,c);
                return;
            }
        }
        if(c + 1 < 256) {
            long ref = get_addr_and_mask(r,c+1);
            long tgt = inc_x_addr_and_mask(src);
            if(ref != tgt) {
                printf("from %lx -> %lx != %lx for %d,%d+1\n",src,ref,tgt,r,c);
                return;
            }
        }
        if(c - 1 >= 0) {
            long ref = get_addr_and_mask(r,c-1);
            long tgt = dec_x_addr_and_mask(src);
            if(ref != tgt) {
                printf("from %lx -> %lx != %lx for %d,%d-1\n",src,ref,tgt,r,c);
                return;
            }
        }
    }
}

int draw_v1(int x0,int x1,int y0,int y1);

void from_center(int dy,int dx)
{
    draw_v1(127,127+dx,95,95+dy);
}

#include <math.h>
int main()
{
    from_center(1,1);
    return 0;
    for(float a=0;a<3.14195*2;a+=3.14159/180*30) {
        from_center((int)(cos(a)*50),(int)(sin(a)*50));
    }
    return 0;
    for(int i=0;i<256;i++) {
        draw_v1(0,i,0,191);
        //draw_v1(255,i,0,191);
    }
    return 0;
    for(int j=0;j<192;j++)
        draw_v1(0,255,0,j);
    for(int j=0;j<192;j++)
        draw3(0,255,0,j);
    return 0;
}
