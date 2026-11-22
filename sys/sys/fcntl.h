#ifndef _SYS_FCNTL_H
#define _SYS_FCNTL_H

#include <sys/uapi/nyx/fcntl.h>

#define O_RDONLY   __KERNEL_O_RDONLY
#define O_WRONLY   __KERNEL_O_WRONLY
#define O_RDWR     __KERNEL_O_RDWR
#define O_ACCMODE  __KERNEL_O_ACCMODE
#define O_CLOEXEC  __KERNEL_O_CLOEXEC
#define O_CLOFORK  __KERNEL_O_CLOFORK
#define O_CREAT    __KERNEL_O_CREAT
#define O_EXCL     __KERNEL_O_EXCL
#define O_NOFOLLOW __KERNEL_O_NOFOLLOW

#endif
