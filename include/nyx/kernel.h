#ifndef _NYX_KERNEL_H
#define _NYX_KERNEL_H

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(*arr))

#define IS_POWER_OF_TWO(x) ((x) && !((x) & ((x) - 1)))

#define KiB (1ULL << 10)
#define MiB (1ULL << 20)
#define GiB (1ULL << 30)
#define TiB (1ULL << 40)

#endif
