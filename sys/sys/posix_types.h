#ifndef _SYS_POSIX_TYPES_H
#define _SYS_POSIX_TYPES_H

#include <sys/uapi/nyx/posix_types.h>

typedef __kernel_off_t   off_t;
typedef __kernel_pid_t   pid_t;
typedef __kernel_mode_t  mode_t;
typedef __kernel_dev_t   dev_t;
typedef __kernel_ino_t   ino_t;
typedef __kernel_nlink_t nlink_t;

#endif
