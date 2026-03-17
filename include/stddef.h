#ifndef _STDDEF_H
#define _STDDEF_H

#include <nyx/types.h>

#undef NULL
#define NULL ((void *) 0)

#ifdef CONFIG_64BIT
typedef u64 size_t;
typedef s64 ssize_t;
#elif defined(CONFIG_32BIT)
typedef u32 size_t;
typedef s32 ssize_t;
#endif

#ifndef SIZE_MAX
#define SIZE_MAX __SIZE_MAX__
#endif

#endif
