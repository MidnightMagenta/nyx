#ifndef _SYS_UAPI_NYX_TYPES_H
#define _SYS_UAPI_NYX_TYPES_H

#include <uapi/asi/posix_types.h>

typedef __kernel_pid_t pid_t;
typedef __kernel_off_t off_t;
typedef unsigned int   mode_t;
typedef unsigned long  dev_t;
typedef unsigned long  ino_t;
typedef unsigned long  nlink_t;

#define PID_MAX 32768

#endif
