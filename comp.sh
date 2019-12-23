#!/bin/bash
gcc -Wall -O3 -g train_f16.c -o train_16_linux || exit 1
gcc -Wall -O3 -g train.c -o train_float_linux || exit 1
./train_16_linux || exit 1
./train_float_linux || exit 1
zcc +zx -vn -O3 -zorg=25600 -startup=31 -clib=sdcc_iy train_f16.c -lm -o train_16 || exit 1
zcc +zx -vn -O3 -zorg=25600 -startup=31 -clib=sdcc_iy train.c -lm -o train || exit 1
appmake +zx -b screen.scr -o screen.tap --blockname screen --org 16384 --noloader || exit 1 
appmake +zx -b test_screen.scr -o test_screen.tap --blockname screen --org 16384 --noloader || exit 1
appmake +zx -b train_CODE.bin -o code.tap --blockname game --org 25600 --noloader || exit 1 
appmake +zx -b train_16_CODE.bin -o code_16.tap --blockname game --org 25600 --noloader || exit 1
cat loader_with_acc.tap code.tap screen.tap test_screen.tap >train_float.tap || exit 1
cat loader_with_acc.tap code_16.tap screen.tap test_screen.tap >train_fixed12.tap || exit 1
