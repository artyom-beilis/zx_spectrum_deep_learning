#ifndef ENABLE_TIMER_H
#define ENABLE_TIMER_H
#ifndef __linux
#pragma output REGISTER_SP = 0xFF58
#pragma output CLIB_MALLOC_HEAP_SIZE = -0xFBFA
#include <z80.h>
#include <string.h>
#include <intrinsic.h>
#include <im2.h>
#include <arch/zx.h>

IM2_DEFINE_ISR(isr)
{
    unsigned *clock_ptr=(void*)(23672);
    unsigned char *high_ptr=(void*)(23674);
    if(*clock_ptr == 0xFFFF) {
        ++*high_ptr;
    }
    ++*clock_ptr;
}
#define TABLE_HIGH_BYTE        ((unsigned int)0xfc)
#define JUMP_POINT_HIGH_BYTE   ((unsigned int)0xfb)

#define UI_256                 ((unsigned int)256)

#define TABLE_ADDR             ((void*)(TABLE_HIGH_BYTE*UI_256))
#define JUMP_POINT             ((unsigned char*)( (unsigned int)(JUMP_POINT_HIGH_BYTE*UI_256) + JUMP_POINT_HIGH_BYTE ))

long get_time()
{
    return (*(long*)(23674)<<16) + *(unsigned *)(23672);
}

long timer;

void start_timer()
{
    timer = get_time();
}
long stop_timer()
{
    return (get_time()-timer);
}

void enable_timer()
{
  memset( TABLE_ADDR, JUMP_POINT_HIGH_BYTE, 257 );

  z80_bpoke( JUMP_POINT,   195 );
  z80_wpoke( JUMP_POINT+1, (unsigned int)isr );

  im2_init( TABLE_ADDR );

  intrinsic_ei();

}

#else
long get_time()
{
    return 0;
}

long timer;

void start_timer()
{
}
long stop_timer()
{
    return 1;
}

void enable_timer()
{
}
#endif

#endif
