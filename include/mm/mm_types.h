#ifndef _MM_MM_TYPES_H
#define _MM_MM_TYPES_H

#include <nyx/list.h>
#include <nyx/types.h>

#include <asi/bitops.h>

#define PG_reserved 0

#define PageReserved(page) test_bit(PG_reserved, &(page)->flags)

#define SetPageReserved(page) set_bit(PG_reserved, &(page)->flags)

#define ClearPageReserved(page) clear_bit(PG_reserved, &(page)->flags)

#define KM_SLEEP   (1 << 0)
#define KM_NOSLEEP 0

struct page {
    u64              flags;
    struct list_head list;

    union {
        struct {
            struct kmem_cache_s *kmem_cache;
            struct kmem_slab_s  *kmem_slab;
        };
    };
};

#endif
