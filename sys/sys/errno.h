#ifndef _ERR_H
#define _ERR_H

#include <asi/errno.h>

#define EPERM         __KERNEL_EPERM         /* Operation not permitted */
#define ENOENT        __KERNEL_ENOENT        /* No such file or directory */
#define ESRCH         __KERNEL_ESRCH         /* No such process */
#define EINTR         __KERNEL_EINTR         /* Interrupted system call */
#define EIO           __KERNEL_EIO           /* I/O error */
#define ENXIO         __KERNEL_ENXIO         /* No such device or address */
#define E2BIG         __KERNEL_E2BIG         /* Argument list too long */
#define ENOEXEC       __KERNEL_ENOEXEC       /* Exec format error */
#define EBADF         __KERNEL_EBADF         /* Bad file number */
#define ECHILD        __KERNEL_ECHILD        /* No child processes */
#define EAGAIN        __KERNEL_EAGAIN        /* Try again */
#define ENOMEM        __KERNEL_ENOMEM        /* Out of memory */
#define EACCES        __KERNEL_EACCES        /* Permission denied */
#define EFAULT        __KERNEL_EFAULT        /* Bad address */
#define ENOTBLK       __KERNEL_ENOTBLK       /* Block device required */
#define EBUSY         __KERNEL_EBUSY         /* Device or resource busy */
#define EEXIST        __KERNEL_EEXIST        /* File exists */
#define EXDEV         __KERNEL_EXDEV         /* Cross-device link */
#define ENODEV        __KERNEL_ENODEV        /* No such device */
#define ENOTDIR       __KERNEL_ENOTDIR       /* Not a directory */
#define EISDIR        __KERNEL_EISDIR        /* Is a directory */
#define EINVAL        __KERNEL_EINVAL        /* Invalid argument */
#define ENFILE        __KERNEL_ENFILE        /* File table overflow */
#define EMFILE        __KERNEL_EMFILE        /* Too many open files */
#define ENOTTY        __KERNEL_ENOTTY        /* Not a typewriter */
#define ETXTBSY       __KERNEL_ETXTBSY       /* Text file busy */
#define EFBIG         __KERNEL_EFBIG         /* File too large */
#define ENOSPC        __KERNEL_ENOSPC        /* No space left on device */
#define ESPIPE        __KERNEL_ESPIPE        /* Illegal seek */
#define EROFS         __KERNEL_EROFS         /* Read-only file system */
#define EMLINK        __KERNEL_EMLINK        /* Too many links */
#define EPIPE         __KERNEL_EPIPE         /* Broken pipe */
#define EDOM          __KERNEL_EDOM          /* Math argument out of domain of func */
#define ERANGE        __KERNEL_ERANGE        /* Math result not representable */
#define EADDRNOTAVAIL __KERNEL_EADDRNOTAVAIL /* Address not available */
#define ENOSYS        __KERNEL_ENOSYS        /* No syscall */
#define ENAMETOOLONG  __KERNEL_ENAMETOOLONG  /* Name too long */
#define ELOOP         __KERNEL_ELOOP         /* Is a cycle */
#define ENOTEMPTY     __KERNEL_ENOTEMPTY     /* Directory not empty */

#define MAX_ERRNO       __KERNEL_MAX_ERRNO
#define IS_ERR_VALUE(x) __KERNEL_IS_ERR_VALUE(x)

#endif
