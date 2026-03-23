#ifndef _MM_PMM_H
#define _MM_PMM_H

#include <asi/page.h>
#include <mm/mm_types.h>
#include <nyx/types.h>
#include <stddef.h>
#include <string.h>

#define PMT_NONE    0
#define PMT_KERNEL  1
#define PMZ_DMA_ANY 0
#define PMZ_DMA_ISA 1
#define PMZ_DMA_32  2
#define PMF_ZERO    (1 << 7)


#define PM_TYPE_MASK 0x0F
#define PM_ZONE_MASK 0x40

enum {
    PM_CONSTRAINT_MAX,
    PM_CONSTRAINT_MIN,
    PM_CONSTRAINT_RANGE,
};

struct pm_constraint {
    u32 type;
    union {
        struct {
            phys_addr_t min;
            phys_addr_t max;
        };
    };
};

extern struct page *page_map;

int pm_reserve_region(phys_addr_t addr, size_t count);
int pm_free_region(phys_addr_t addr, size_t count);

struct page *__pm_alloc_pages(u64 flags, u64 order);
phys_addr_t  __pm_get_pages(u64 flags, u64 order);
#define pm_alloc_page(flags) __pm_get_pages((priority), 0);

void pm_free_pages(phys_addr_t addr);

phys_addr_t __pm_get_pages_ex(u64                               flags,
                              u64                               order,
                              const struct pm_constraint *const constraint,
                              size_t                            constraint_cnt);

int pm_is_free(phys_addr_t addr);
int pm_is_reserved(phys_addr_t addr);

enum pm_stat {
    PM_STAT_MIN_ALIGN,
    PM_STAT_MAX_ALIGN,
    PM_STAT_TOTAL,
    PM_STAT_FREE,
    PM_STAT_USED,
};

size_t pm_getstat(enum pm_stat stat);

#endif
