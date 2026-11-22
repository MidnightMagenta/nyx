#ifndef _SYS_UAPI_NYX_FCNTL_H
#define _SYS_UAPI_NYX_FCNTL_H

#define __KERNEL_O_RDONLY   0
#define __KERNEL_O_WRONLY   1
#define __KERNEL_O_RDWR     2
#define __KERNEL_O_ACCMODE  3
#define __KERNEL_O_CLOEXEC  (1 << 3)
#define __KERNEL_O_CLOFORK  (1 << 4)
#define __KERNEL_O_CREAT    (1 << 5)
#define __KERNEL_O_EXCL     (1 << 6)
#define __KERNEL_O_NOFOLLOW (1 << 7)

#endif
