#ifndef _SYS_UAPI_NYX_MMAN_H
#define _SYS_UAPI_NYX_MMAN_H

#define __KERNEL_PROT_READ  (1 << 0)
#define __KERNEL_PROT_WRITE (1 << 1)
#define __KERNEL_PROT_EXEC  (1 << 2)
#define __KERNEL_PROT_NONE  (1 << 3)

#define __KERNEL_MAP_PRIVATE   (1 << 0)
#define __KERNEL_MAP_SHARED    (1 << 1)
#define __KERNEL_MAP_ANONYMOUS (1 << 2)
#define __KERNEL_MAP_ANON      MAP_ANONYMOUS
#define __KERNEL_MAP_FIXED     (1 << 3)

#endif
