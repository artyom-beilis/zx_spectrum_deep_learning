echo "float16"
zcc  +zx -vn -DUSE_F16 -I../float16/include -I. -I.. -O3 -g -zorg=32768 mandelbrot_real.c ../float16/z80/float_z80.asm -clib=sdcc_iy -o mandelbrot  || exit 1
appmake +zx -b mandelbrot_CODE.bin -o mandelbrot_float16.tap --org 32768  || exit 1
echo "ok"

echo "fixed"
zcc  +zx -vn -DUSE_FIXED -DUSE_FIXED88=1 -I. -I.. -O3 -g -zorg=32768 mandelbrot_real.c -clib=sdcc_iy -o mandelbrot  || exit 1
appmake +zx -b mandelbrot_CODE.bin -o mandelbrot_fixed8.8.tap --org 32768 || exit 1
echo ok

echo "float"
zcc  +zx -vn -DUSE_FLOAT -I. -I.. -O3 -g -zorg=32768 mandelbrot_real.c  -lm -clib=sdcc_iy -o mandelbrot  || exit 1
appmake +zx -b mandelbrot_CODE.bin -o mandelbrot_float32.tap --org 32768  || exit 1
echo "ok"


