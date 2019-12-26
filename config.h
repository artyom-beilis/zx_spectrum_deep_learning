#ifndef MNIST_CONFIG_H
#define MNIST_CONFIG_H

//#define VERY_FAST
#ifdef VERY_FAST

#define DATA_SIZE  64
#define INPSIZE 8
#define ITER_SIZE 4
#define KERNELS 4
#define KSIZE   3
#define CLASS_NO 2
#define EPOCHS 2

#else

#define DATA_SIZE  64
#define INPSIZE 8
#define ITER_SIZE 1
#define KERNELS 12
#define KSIZE   3
#define CLASS_NO 10
#define EPOCHS 5

#endif

#endif

