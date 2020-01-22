#include <stdio.h>
#include <stdlib.h>
#include <float16.h>
#include "sph.h"
extern unsigned long  get_addr_and_mask(int r,int c);
extern unsigned long  inc_x_addr_and_mask(unsigned long addr_and_mask) __z88dk_fastcall;
extern unsigned long  dec_x_addr_and_mask(unsigned long addr_and_mask) __z88dk_fastcall;
extern unsigned long  inc_y_addr_and_mask(unsigned long addr_and_mask) __z88dk_fastcall;
extern int draw_line(int x0,int x1,int y0,int y1);
extern int plot(int r,int c);
extern void clear_screen();

void from_center(int dy,int dx)
{
    draw_line(127,127+dx,95,95+dy);
}

unsigned char points[VERTS][2];

void make_points()
{
    short factor=f16_from_int(50);
    for(int i=0;i<VERTS;i++) {
        points[i][0]=f16_int(f16_mul(vertices[i][1],factor)) + 127;
        points[i][1]=f16_int(f16_mul(vertices[i][2],factor)) + 97;
    }
}

void plot_sph()
{
    for(int i=0;i<EDGES;i++) {
        int p1 = edges[i][0];
        int p2 = edges[i][1];
        draw_line(points[p1][0],points[p2][0],points[p1][1],points[p2][1]);
    }
}

/*
#include <math.h>
void test_star()
{
    for(int a=0;a<360;a+=30) {
        float ang = a*3.14159f/180;
        int dx = 20*sin(ang);
        int dy = 20*cos(ang);
        from_center(dx,dy);
    }
}*/

int main()
{
    make_points();
    for(int j=0;j<10;j++) {
        clear_screen();
        //for(int i=0;i<100;i++)
        //    plot(rand(),rand());
        plot_sph();
    }
    return 0;
}
