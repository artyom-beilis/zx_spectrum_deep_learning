#!/bin/bash
rm -f *.tap
rm -f *.bin
rm -f *_linux 
ZXTEMP=/tmp/zx_temp/
rm -fr $ZXTEMP
mkdir -p $ZXTEMP

# Reference Run
gcc -Wall -O3 -g  train.c  -o train_float_linux || exit 1
./train_float_linux || exit 1 # also creates 
gcc -Wall -O3 -g -I float16/include train_float16.c float16/c_src/float16.c  -o train_half_linux || exit 1
./train_half_linux || exit 1 # also creates 
gcc -Wall -O3 -g train_f16.c -o train_fixed16_linux || exit 1
./train_fixed16_linux || exit 1 

# zcc build
zcc +zx -vn -O3 -zorg=25600 -startup=31 -clib=sdcc_iy train.c  -lm -o $ZXTEMP/train || exit 1
appmake +zx -b $ZXTEMP/train_CODE.bin -o code.tap --blockname dl --org 25600 --noloader || exit 1 

zcc +zx -vn -O3 -zorg=25600 -startup=0 -clib=sdcc_iy train_f16.c -lm -o $ZXTEMP/train_16 || exit 1
appmake +zx -b $ZXTEMP/train_16_CODE.bin -o code_16.tap --blockname dl --org 25600 --noloader || exit 1

zcc +zx -vn -O3 -g -zorg=25600 -startup=31 -clib=sdcc_iy -Ifloat16/include float16/z80/float_z80.asm train_float16.c  -lm -o $ZXTEMP/train_half || exit 1
appmake +zx -b $ZXTEMP/train_half_CODE.bin -o code_half.tap --blockname dl --org 25600 --noloader || exit 1 


# make asm loader for data screen block
z80asm -b -o$ZXTEMP/load_data.bin load.asm || exit 1
python bin_to_data.py $ZXTEMP/load_data.bin >$ZXTEMP/load_screen.bas

# make C loader
gcc -Wall print_epochs_no.c -o $ZXTEMP/print_epochs_no || exit 1
$ZXTEMP/print_epochs_no | cat loader.bas - >$ZXTEMP/c_loader.bas 
bas2tap -sloader -a1 $ZXTEMP/c_loader.bas c_code_loader.tap

cat train_screen_{header,body}.tap test_screen_{header,body}.tap >train_test.tap
cat train_screen_body.tap test_screen_body.tap >train_test_body.tap

# make tapes for all
cat c_code_loader.tap code.tap train_test.tap >train_C_float.tap
cat c_code_loader.tap code_half.tap train_test.tap >train_C_float16.tap
cat c_code_loader.tap code_16.tap train_test.tap >train_C_fixed.tap


# make basic code
bas2tap -slearn -a1 <(cat train.bas | grep -v ' 36 LET ')  basic_code.tap || exit 1
cat basic_code.tap train_test.tap >train_basic.tap
bas2tap -slearn <(cat $ZXTEMP/load_screen.bas train.bas | sed 's/LOAD.*/RANDOMIZE USR 23296/') basic_code_for_tobos.tap || exit 1
cat basic_code_for_tobos.tap train_test_body.tap >train_basic_tobos.tap



