#ifndef _ASI_GENERIC_TYPES_H
#define _ASI_GENERIC_TYPES_H

typedef unsigned char     __u8;
typedef unsigned short    __u16;
typedef unsigned int      __u32;
typedef unsigned long int __u64;
typedef signed char       __s8;
typedef short             __s16;
typedef int               __s32;
typedef long int          __s64;

#ifdef CONFIG_32BIT
typedef __u32 __phys_addr_t;
typedef __u32 __virt_addr_t;
#elif defined(CONFIG_64BIT)
typedef __u64 __phys_addr_t;
typedef __u64 __virt_addr_t;
#endif

#endif
