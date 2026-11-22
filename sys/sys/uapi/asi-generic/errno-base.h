#ifndef _UAPI_ASI_GENERIC_ERR_BASE_H
#define _UAPI_ASI_GENERIC_ERR_BASE_H

#define __KERNEL_EPERM         1  /* Operation not permitted */
#define __KERNEL_ENOENT        2  /* No such file or directory */
#define __KERNEL_ESRCH         3  /* No such process */
#define __KERNEL_EINTR         4  /* Interrupted system call */
#define __KERNEL_EIO           5  /* I/O error */
#define __KERNEL_ENXIO         6  /* No such device or address */
#define __KERNEL_E2BIG         7  /* Argument list too long */
#define __KERNEL_ENOEXEC       8  /* Exec format error */
#define __KERNEL_EBADF         9  /* Bad file number */
#define __KERNEL_ECHILD        10 /* No child processes */
#define __KERNEL_EAGAIN        11 /* Try again */
#define __KERNEL_ENOMEM        12 /* Out of memory */
#define __KERNEL_EACCES        13 /* Permission denied */
#define __KERNEL_EFAULT        14 /* Bad address */
#define __KERNEL_ENOTBLK       15 /* Block device required */
#define __KERNEL_EBUSY         16 /* Device or resource busy */
#define __KERNEL_EEXIST        17 /* File exists */
#define __KERNEL_EXDEV         18 /* Cross-device link */
#define __KERNEL_ENODEV        19 /* No such device */
#define __KERNEL_ENOTDIR       20 /* Not a directory */
#define __KERNEL_EISDIR        21 /* Is a directory */
#define __KERNEL_EINVAL        22 /* Invalid argument */
#define __KERNEL_ENFILE        23 /* File table overflow */
#define __KERNEL_EMFILE        24 /* Too many open files */
#define __KERNEL_ENOTTY        25 /* Not a typewriter */
#define __KERNEL_ETXTBSY       26 /* Text file busy */
#define __KERNEL_EFBIG         27 /* File too large */
#define __KERNEL_ENOSPC        28 /* No space left on device */
#define __KERNEL_ESPIPE        29 /* Illegal seek */
#define __KERNEL_EROFS         30 /* Read-only file system */
#define __KERNEL_EMLINK        31 /* Too many links */
#define __KERNEL_EPIPE         32 /* Broken pipe */
#define __KERNEL_EDOM          33 /* Math argument out of domain of func */
#define __KERNEL_ERANGE        34 /* Math result not representable */
#define __KERNEL_EADDRNOTAVAIL 35 /* Address not available */
#define __KERNEL_ENOSYS        36 /* No syscall */
#define __KERNEL_ENAMETOOLONG  37 /* Name too long */
#define __KERNEL_ELOOP         38 /* Is a cycle */
#define __KERNEL_ENOTEMPTY     39 /* Directory not empty */

#define __KERNEL_MAX_ERRNO       4095
#define __KERNEL_IS_ERR_VALUE(x) ((x) >= (unsigned long) -MAX_ERRNO)

#endif
