echo "float16"
g++ -std=c++11 -DUSE_F16 -Wall sphere.cpp && ./a.out >sph.h 
zcc  +zx -vn -DUSE_F16 -I. -I.. -O3 -g -zorg=32768 line.c plot_utils.asm float_z80.asm float16.c -clib=sdcc_iy -o line  || exit 1
appmake +zx -b line_CODE.bin -o line_float16.tap --org 32768 --clearaddr 26367 || exit 1
echo "ok"

echo "fixed"
g++ -std=c++11 -DUSE_FIXED -Wall sphere.cpp && ./a.out >sph.h 
zcc  +zx -vn -DUSE_FIXED -I. -I.. -O3 -g -zorg=32768 line.c plot_utils.asm -clib=sdcc_iy -o line  || exit 1
appmake +zx -b line_CODE.bin -o line_fixed4.12.tap --org 32768 --clearaddr 26367 || exit 1
echo ok

echo "float"
g++ -std=c++11 -DUSE_FLOAT -Wall sphere.cpp && ./a.out >sph.h 
zcc  +zx -vn -DUSE_FLOAT -I. -I.. -O3 -g -zorg=32768 line.c plot_utils.asm -lm -clib=sdcc_iy -o line  || exit 1
appmake +zx -b line_CODE.bin -o line_float32.tap --org 32768 --clearaddr 26367 || exit 1
echo "ok"


