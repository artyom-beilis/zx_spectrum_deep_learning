#include <stdio.h>


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


void draw2(int x0,int x1,int y0,int y1)
{
    int dx=x1-x0;
    int dy=y1-y0;
    int D=dy-dx/2;
    int y=y0;
    int x=x0;
    static int printed;
    while(x<=x1) {
        plot(y,x);
        if(D > 0) {
            y++;

            D=D-dx;
        }
        D=D+dy;
        x++;
    }
}
void draw_v1(int x0,int x1,int y0,int y1);
void draw(int x0,int x1,int y0,int y1)
{
    int dx=x1-x0;
    int dy=y1-y0;
    int D=2*dy-dx;
    int y=y0;
    int x=x0;
    unsigned char *ptr=(unsigned char *)(16384);
    ptr += (y >> 6) * (32*64);
    ptr += (y & 7) * 256 + ((y&64) >> 3) * 32;
    ptr += x>>3;
    unsigned char or_mask = 1 << (7-(x&7));
    while(x<=x1) {
        *ptr |= or_mask;
        if(D > 0) {
            y++;
            if((y & 63) == 0) {
                ptr += 32;
            }
            else if((y & 7) == 0) {
                ptr -= 2048-256-32;
            }
            else
                ptr += 256;

            D=D-2*dx;
        }
        D=D+2*dy;
        x++;
        if((x & 7) == 0) {
            ptr++;
            or_mask = 0x80;
        }
        else
            or_mask >>= 1;
    }
}

int main()
{
    for(int j=0;j<192;j+=16)
        draw2(0,255,0,j);
    return 0;
}
