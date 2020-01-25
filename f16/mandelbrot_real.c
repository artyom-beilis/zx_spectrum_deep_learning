#include "enable_timer.h"
#include "real.h"
#include <stdio.h>
#ifdef __linux
unsigned char screen[256*192];
void plot(int r,int c)
{
    if(r < 0 || r>=192)
        return;
    if(c<0 || c>=256)
        return;
    screen[r*256+c]=255;
}
void show_image()
{
    FILE *f=fopen("mand.pgm","w");
    fprintf(f,"P5\n256 192\n255\n");
    fwrite(screen,sizeof(screen),1,f);
    fclose(f);
}

#else

void plot(int r,int c)
{
    if(r < 0 || r>=192)
        return;
    if(c < 0 || c>=256)
        return;
    unsigned char *ptr = (unsigned char *)(16384);
    ptr += (r >> 6) * (32*64);
    r=r&63;
    ptr += (r & 7) * 256 + (r >> 3)*32;
    ptr += (c >> 3);
    c=c & 7;
    unsigned  char v=*ptr;
    *ptr = v | (1<<(7-c));
}

#define show_image()

#endif


int main()
{
    printf("\nmandelbrot " real_name "  \n");
    enable_timer();
    start_timer();
    #ifndef USE_FIXED
    real_t xscale = real_div(real_from_int(7),real_from_int(512));
    real_t yscale = real_div(real_from_int(2),real_from_int(192));
    #endif
    real_t xoffset = real_div(real_from_int(-5),real_from_int(2));
    real_t yoffset = real_neg(real_one);
    real_t limit = real_from_int(4);

    for(int ix=0;ix<256;ix++) {
        for(int iy=0;iy<192;iy++) {
#ifdef USE_FIXED
            real_t x0 = (ix*7 >> 1) + xoffset;
            real_t y0 = real_mul(iy,682) + yoffset;
#else       
            real_t x0 = real_add(real_mul(real_from_int(ix),xscale),xoffset);
            real_t y0 = real_add(real_mul(real_from_int(iy),yscale),yoffset);
#endif      

            real_t x=0,y=0;
            int it = 0;
            int max_it = 50;
            while(1) {
                real_t xx=real_mul(x,x);
                real_t yy=real_mul(y,y);
                if(real_gt(real_add(xx,yy),limit) || it >= max_it) {
                    break;
                }
                real_t xtemp = real_add(real_sub(xx,yy),x0);
                real_t xy=real_mul(x,y);
                y=real_add(real_add(xy,xy),y0);
                x=xtemp;
                it++;
            }
            if(it >= max_it)
                plot(iy,ix);
        }
    }
    show_image();
    printf("\n\nPassed %dm  \n",(int)(stop_timer()/(50*60)));
    return 0;
}
