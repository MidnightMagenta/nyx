#ifndef _MM_MM_TYPES_H
#define _MM_MM_TYPES_H

#include <nyx/atomic.h>
#include <nyx/list.h>
#include <nyx/types.h>

#include <asi/bitops.h>
#include <asi/page_data.h>

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

#define __GFP_DMA     (1 << 0)
#define __GFP_DMA32   (1 << 1)
#define __GFP_HIGHMEM (1 << 2)
#define __GFP_HIGH    (1 << 3)
#define __GFP_WAIT    (1 << 4)
#define __GFP_ZERO    (1 << 5)

#define GFP_DMA      (__GFP_DMA)
#define GFP_DMA32    (__GFP_DMA32)
#define GFP_ATOMIC   (__GFP_HIGH)
#define GFP_KERNEL   (__GFP_WAIT)
#define GFP_USER     (__GFP_WAIT)
#define GFP_HIGHUSER (__GFP_WAIT | __GFP_HIGHMEM)

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

#define VM_READ          (1 << 0)
#define VM_WRITE         (1 << 1)
#define VM_EXEC          (1 << 2)
#define VM_USER          (1 << 3)
#define VM_CACHE_DISABLE (1 << 4)

struct vma_region_struct {
    struct list_head list;

    virt_addr_t start, end;
    u32         prot;
    u32         flags;

    void *private;
};

struct mm_struct {
    pgd_t           *pgd;
    struct list_head vma_regions;

    atomic_t refcount;
    atomic_t user_count;
};

#endif
