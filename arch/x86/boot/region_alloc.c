#include "region_alloc.h"
#include "boot_utils.h"
#include "printb.h"
#include <nyx/align.h>
#include <nyx/stddef.h>
#include <nyx/types.h>

#define MAX_REGIONS 256

static struct memregion free_regions[MAX_REGIONS];
static size_t           region_count = 1;

static void ra_sort(struct memregion *const r, size_t *n) {
    struct memregion temp = {0};
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

static void ra_merge(struct memregion *const r, size_t *n) {
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

#define ra_sort_and_merge(r, n)                                                                                        \
    ra_sort(r, n);                                                                                                     \
    ra_merge(r, n);

int ra_add_region(struct memregion region) {
    if (region_count >= MAX_REGIONS) { return 1; }
    free_regions[region_count++] = region;
    ra_sort_and_merge(free_regions, &region_count);
    return 0;
}

int ra_subtract_region(struct memregion region) {
    u64 sb = region.base;
    u64 se = region.base + region.size;

    for (size_t i = 0; i < region_count; ++i) {
        struct memregion *r = &free_regions[i];

        u64 rb = r->base;
        u64 re = r->base + r->size;

        if (se <= rb || sb >= re) { continue; }

        if (sb <= rb && se >= re) {
            memmoveb(&free_regions[i], &free_regions[i + 1], (region_count - i - 1) * sizeof(*r));
            region_count--;
            i--;
            continue;
        }

        if (sb <= rb) {
            r->base = se;
            r->size = re - se;
            continue;
        }

        if (se >= re) {
            r->size = sb - rb;
            continue;
        }

        if (region_count >= MAX_REGIONS) return 1;

        memmoveb(&free_regions[i + 2], &free_regions[i + 1], (region_count - i - 1) * sizeof(*r));

        free_regions[i + 1].base = se;
        free_regions[i + 1].size = re - se;

        r->size = sb - rb;

        region_count++;
        return 0;
    }

    return 0;
}

extern int ra_init_imp(u64 bi, struct memregion *free_regions, size_t *region_count, const size_t region_array_size);

int ra_init(u64 bi) {
    return ra_init_imp(bi, free_regions, &region_count, sizeof(free_regions));
}

struct memregion ra_get_region(u64 size, u64 align) {
    int try_lowmem = 0;

    if (size == 0 || align == 0) { return (struct memregion) {0, 0}; }

retry:
    for (size_t i = 0; i < region_count; ++i) {
        struct memregion r = free_regions[i];

        u64 rb = ALIGN_UP(r.base, align);
        u64 re = r.base + r.size;

        // prefer avoiding low memory
        if (!try_lowmem && rb < 0x100000) { rb = 0x100000; }

        // alignment pushed up past the region
        if (rb >= re) { continue; }

        // not enough space in the region
        if (re - rb < size) { continue; }

        struct memregion out = {rb, size};
        ra_subtract_region(out);
        return out;
    }

    if (!try_lowmem) {
        try_lowmem = 1;
        goto retry;
    }
    return (struct memregion) {0, 0};
}

void ra_dump_regions() {
    for (size_t i = 0; i < region_count; i++) {
        printb("region %d:\n  base: 0x%lx\n  size: 0x%lx\n", i, free_regions[i].base, free_regions[i].size);
    }
}
