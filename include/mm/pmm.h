#ifndef _MM_PMM_H
#define _MM_PMM_H

#include <asi/page.h>
#include <mm/mm_types.h>
#include <nyx/limits.h>
#include <nyx/stddef.h>
#include <nyx/string.h>
#include <nyx/types.h>

#define PMT_NONE    0
#define PMT_KERNEL  1
#define PMZ_DMA_ANY 0
#define PMZ_DMA_ISA 1
#define PMZ_DMA_32  2
#define PMF_ZERO    (1 << 7)

#define PM_TYPE_MASK 0x0F
#define PM_ZONE_MASK 0x40

#define PM_INVALID_ADDR PHYS_ADDR_MAX;

// FIXME: implement __pm_get_pages_ex and __pm_alloc_pages_ex
// enum {
//     PM_CONSTRAINT_MAX,
//     PM_CONSTRAINT_MIN,
//     PM_CONSTRAINT_RANGE,
// };
//
// struct pm_constraint {
//     u32 type;
//     union {
//         struct {
//             phys_addr_t min;
//             phys_addr_t max;
//         };
//     };
// };

extern struct page *page_map;

phys_addr_t  pm_page_to_phys(struct page *pg);
struct page *pm_phys_to_page(phys_addr_t addr);

int pm_reserve_region(phys_addr_t addr, size_t count);
int pm_free_region(phys_addr_t addr, size_t count);

struct page       *__pm_alloc_pages(u64 flags, u64 order);
inline phys_addr_t __pm_get_pages(u64 flags, u64 order) {
    return pm_page_to_phys(__pm_alloc_pages(flags, order));
}
#define pm_get_page(flags) __pm_get_pages((priority), 0);

void pm_free_pages(phys_addr_t addr);

// FIXME: implement __pm_get_pages_ex and __pm_alloc_pages_ex
// phys_addr_t __pm_get_pages_ex(u64                               flags,
//                               u64                               order,
//                               const struct pm_constraint *const constraint,
//                               size_t                            constraint_cnt);

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
