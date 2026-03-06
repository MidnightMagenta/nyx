#include <mm/memblock.h>
#include <nyx/errno.h>
#include <nyx/linkage.h>

#define MEMBLOCK_INIT_REGIONS 128

static struct memblock_region memblock_memory_init_regions[MEMBLOCK_INIT_REGIONS] __initdata;
static struct memblock_region memblock_reserved_init_regions[MEMBLOCK_INIT_REGIONS] __initdata;

struct memblock memblock __initdata = {
        .memory.regions = memblock_memory_init_regions,
        .memory.cnt     = 1,
        .memory.max     = MEMBLOCK_INIT_REGIONS,

        .reserved.regions = memblock_reserved_init_regions,
        .reserved.cnt     = 1,
        .reserved.max     = MEMBLOCK_INIT_REGIONS,

        .bottom_up = false,
};

static void __init memblock_sort(struct memblock_region *const r, size_t *n) {
    struct memblock_region temp = {0};
    if (*n == 0) { return; }

    for (size_t i = 1; i < *n; ++i) {
        temp     = r[i];
        size_t j = i;

        while (j > 0 && r[j - 1].base > temp.base) {
            r[j] = r[j - 1];
            j--;
        }
        r[j] = temp;
    }
}

static void __init memblock_merge_regions(struct memblock_region *const r, size_t *n) {
    size_t wr = 0;
    if (*n == 0) { return; }

    for (size_t rd = 1; rd < *n; ++rd) {
        u64 prev_end = r[wr].base + r[wr].size;
        u64 cur_end  = r[rd].base + r[rd].size;

        if (r[rd].base <= prev_end) {
            if (cur_end > prev_end) { r[wr].size = cur_end - r[wr].base; }
        } else {
            wr++;
            r[wr] = r[rd];
        }
    }

    *n = wr + 1;
}

static inline void __init memblock_sort_and_merge(struct memblock_region *const regions, size_t *n) {
    memblock_sort(regions, n);
    memblock_merge_regions(regions, n);
}

int memblock_reserve_region(phys_addr_t addr, size_t size) {
    return 0;
}

int memblock_unreserve_region(phys_addr_t addr, size_t size) {
    return 0;
}
