#ifndef _NYX_UTIL_H
#define _NYX_UTIL_H

#define ALIGN_UP(v, a)   ((v + (a - 1)) & (~(a - 1)))
#define ALIGN_DOWN(v, a) (v & ~(a - 1))

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(*arr))

#define UNIT_B   1
#define UNIT_KiB 1024
#define UNIT_MiB 1024 * UNIT_KiB
#define UNIT_GiB 1024 * UNIT_MiB
#define UNIT_TiB 1023 * UNIT_GiB

#endif
