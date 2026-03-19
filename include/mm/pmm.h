#ifndef _MM_PMM_H
#define _MM_PMM_H

#include <asi/page.h>
#include <nyx/types.h>
#include <stddef.h>
#include <string.h>

#define PM_ZERO           (1 << 0)
#define PM_ALLOC_BOTTOMUP (1 << 1)
#define PM_ALLOC_TOPDOWN  (0 << 2)
#define PM_ALLOC_RELAX    (1 << 3)

enum {
    PM_CONSTRAINT_MAX,
    PM_CONSTRAINT_MIN,
    PM_CONSTRAINT_RANGE,
    PM_CONSTRAINT_ALIGN,
};

struct pm_constraint {
    u32 type;
    union {
        phys_addr_t addr;
        struct {
            phys_addr_t min;
            phys_addr_t max;
        };
        phys_addr_t align;
    };
};

int pm_reserve_region(phys_addr_t addr, size_t count);
int pm_free_region(phys_addr_t addr, size_t count);

phys_addr_t __pm_alloc_page();
void        pm_free_page(phys_addr_t addr);
phys_addr_t __pm_alloc_pages(size_t                            count,
                             u32                               flags,
                             const struct pm_constraint *const constraint,
                             size_t                            constraint_cnt);
void        pm_free_pages(phys_addr_t addr, size_t count);

inline phys_addr_t pm_alloc_page() {
    phys_addr_t page;

    page = __pm_alloc_page();
    if (page) { memset((void *) page, 0, PAGE_SIZE); }
    return page;
}

inline phys_addr_t pm_alloc_pages(size_t count) {
    return __pm_alloc_pages(count, PM_ALLOC_TOPDOWN | PM_ZERO, NULL, 0);
}

inline phys_addr_t pm_aligned_alloc_pages(size_t count, phys_addr_t align) {
    struct pm_constraint constraint = {
            .type  = PM_CONSTRAINT_ALIGN,
            .align = align,
    };
    return __pm_alloc_pages(count, PM_ALLOC_TOPDOWN | PM_ZERO, &constraint, 1);
}

int pm_is_free(phys_addr_t addr);
int pm_is_reserved(phys_addr_t addr);

enum pm_stat {
    PM_STAT_MIN_ALIGN,
    PM_STAT_MAX_ALIGN,
    PM_STAT_TOTAL,
    PM_STAT_FREE,
    PM_STAT_USED,
    PM_STAT_FREE_LOMEM,
    PM_STAT_USED_LOMEM,
    PM_STAT_FREE_NORMAL,
    PM_STAT_USED_NORMAL,
    PM_STAT_FREE_HIMEM,
    PM_STAT_USED_HIMEM,
};

size_t pm_getstat(enum pm_stat stat);

#endif
