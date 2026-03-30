#ifndef _MM_MM_TYPES_H
#define _MM_MM_TYPES_H

#include <nyx/types.h>

#define PAGE_FLAG_HEAD (1 << 0)
#define PAGE_FLAG_TAIL (1 << 1)
#define PAGE_FLAG_SLAB (1 << 2)

#define KM_SLEEP   (1 << 0)
#define KM_NOSLEEP 0

struct page {
    u64 flags;
    u16 order;
    u8  _pad[6];

    union {
        struct page *head;

        struct {
            struct kmem_cache_s *kmem_cache;
            struct kmem_slab_s  *kmem_slab;
        };
    };
};

#endif
