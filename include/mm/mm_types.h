#ifndef _MM_MM_TYPES_H
#define _MM_MM_TYPES_H

#include <nyx/list.h>
#include <nyx/types.h>

#include <asi/bitops.h>

#define PG_reserved (1 << 0)
#define PG_buddy    (1 << 1)
#define PG_slab     (1 << 2)
#define PG_pgtable  (1 << 3)

#define PageReserved(page) test_bit(PG_reserved, &(page)->flags)
#define PageBuddy(page)    test_bit(PG_buddy, &(page)->flags)
#define PageSlab(page)     test_bit(PG_slab, &(page)->flags)
#define PagePgtable(page)  test_bit(PG_pgtable, &(page)->flags)

#define SetPageReserved(page) set_bit(PG_reserved, &(page)->flags)
#define SetPageBuddy(page)    set_bit(PG_buddy, &(page)->flags)
#define SetPageSlab(page)     set_bit(PG_slab, &(page)->flags)
#define SetPagePgtable(page)  set_bit(PG_pgtable, &(page)->flags)

#define ClearPageReserved(page) clear_bit(PG_reserved, &(page)->flags)
#define ClearPageBuddy(page)    clear_bit(PG_buddy, &(page)->flags)
#define ClearPageSlab(page)     clear_bit(PG_slab, &(page)->flags)
#define ClearPagePgtable(page)  clear_bit(PG_pgtable, &(page)->flags)

struct page {
    u64              flags;
    struct list_head list;

    int zone_id;
    u64 private;

    union {
        struct {
            struct kmem_cache_s *kmem_cache;
            struct kmem_slab_s  *kmem_slab;
        };
    };
};

#endif
