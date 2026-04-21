#ifndef _MM_MM_TYPES_H
#define _MM_MM_TYPES_H

#include <nyx/limits.h>
#include <nyx/list.h>
#include <nyx/types.h>

#include <asi/bitops.h>

#define INVALID_PHYS_ADDR PHYS_ADDR_MAX

#define PG_reserved (1 << 0)
#define PG_buddy    (1 << 1)

#define PageReserved(page) test_bit(PG_reserved, &(page)->flags)
#define PageBuddy(page)    test_bit(PG_buddy, &(page)->flags)

#define SetPageReserved(page) set_bit(PG_reserved, &(page)->flags)
#define SetPageBuddy(page)    set_bit(PG_buddy, &(page)->flags)

#define ClearPageReserved(page) clear_bit(PG_reserved, &(page)->flags)
#define ClearPageBuddy(page)    clear_bit(PG_buddy, &(page)->flags)

struct page {
    u64              flags;
    struct list_head list;

    int zone;

    union {
        int buddy_order;
        struct {
            struct kmem_cache_s *kmem_cache;
            struct kmem_slab_s  *kmem_slab;
        };
    };
};

#endif
