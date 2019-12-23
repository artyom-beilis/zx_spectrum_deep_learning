#!/bin/bash
#zcc +zx -vn -S -z80-verb -O3 -zorg=25600 -startup=31 -clib=sdcc_iy train.c -lm -o train.s

zcc +zx -vn -O3 -zorg=25600 -startup=31 -clib=sdcc_iy train_f16.c -lm -o train
#zcc +zx -vn -O3 -zorg=25600 -startup=31 -clib=sdcc_iy train.c -lm -o train

#zcc +zx -vn -O3 -zorg=25600 -startup=31 -clib=sdcc_iy train.c  -fp-mode=ieee -lmath32 -pragma-define:CLIB_32BIT_FLOAT=1 -o train

appmake +zx -b screen.scr -o screen.tap --blockname screen --org 16384 --noloader
appmake +zx -b test_screen.scr -o test_screen.tap --blockname screen --org 16384 --noloader
appmake +zx -b train_CODE.bin -o code.tap --blockname game --org 25600 --noloader
cat loader_with_acc.tap code.tap screen.tap test_screen.tap >train.tap
