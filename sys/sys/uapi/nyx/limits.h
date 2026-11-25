#ifndef _SYS_UAPI_NYX_LIMITS_H
#define _SYS_UAPI_NYX_LIMITS_H

#include <sys/uapi/nyx/posix_types.h>

#define __KERNEL_PATH_MAX 4096
#define __KERNEL_NAME_MAX 255
#define __KERNEL_OFF_MAX  ((__kernel_off_t) (~0UL >> 1))

#endif
