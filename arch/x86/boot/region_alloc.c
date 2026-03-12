#include "region_alloc.h"
#include "boot_utils.h"
#include "multiboot2.h"
#include <asi/page.h>
#include <nyx/util.h>
#include <stddef.h>

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

        // no overlap
        if (se <= rb || sb >= re) { continue; }

        // fully covered region
        if (sb <= rb && se >= re) {
            memmoveb(&free_regions[i], &free_regions[i + 1], (region_count - i - 1) * sizeof(*r));
            region_count--;
            i--;
            continue;
        }

        // trim front
        if (sb <= rb) {
            r->base = se;
            r->size = re - se;
            continue;
        }

        // trim end
        if (se >= re) {
            r->size = sb - rb;
            continue;
        }

        // split
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

static struct mb2_tag_mmap *get_mmap(u64 bi) {
    unsigned int    bi_size = *(unsigned int *) bi;
    struct mb2_tag *bi_tag  = (struct mb2_tag *) (bi + 8);
    struct mb2_tag *bi_end  = (struct mb2_tag *) (bi + bi_size);

    while (bi_tag <= bi_end && bi_tag->type != MB2_TAG_END) {
        if (bi_tag->type == MB2_TAG_MMAP) { return (struct mb2_tag_mmap *) bi_tag; }
        bi_tag = (struct mb2_tag *) ((char *) bi_tag + ALIGN_UP(bi_tag->size, 8));
    }

    return NULL;
}

extern char __image_start;
extern char __image_end;

int ra_init(u64 bi) {
    struct mb2_tag_mmap *mmap    = get_mmap(bi);
    unsigned int         bi_size = *(unsigned int *) bi;
    struct mb2_tag      *bi_tag  = (struct mb2_tag *) (bi + 8);
    struct mb2_tag      *bi_end  = (struct mb2_tag *) (bi + bi_size);

    memsetb(free_regions, 0, sizeof(free_regions));
    region_count = 1;

    for (u8 *p = (u8 *) mmap->entries; p < (u8 *) mmap + mmap->size; p += mmap->entry_size) {
        struct mb2_mmap_entry *mmap_entry = (struct mb2_mmap_entry *) p;
        if (mmap_entry->type != MB2_MMAP_AVAILABLE) { continue; }
        if (ra_add_region((struct memregion) {.base = mmap_entry->addr, .size = mmap_entry->len}) != 0) { return 1; }
    }

    // reserve memory occupied by the kernel
    if (ra_subtract_region(
                (struct memregion) {.base = ALIGN_DOWN((u64) &__image_start, PAGE_SIZE),
                                    .size = ALIGN_UP(((u64) &__image_end - (u64) &__image_start), PAGE_SIZE)}) != 0) {
        return 1;
    }

    // reserve memory occupied by boot info
    if (ra_subtract_region((struct memregion) {.base = bi, .size = *(u32 *) bi}) != 0) { return 1; }

    // reserve other memory multiboot2 bootloaders don't mark
    while (bi_tag <= bi_end && bi_tag->type != MB2_TAG_END) {
        if (bi_tag->type == MB2_TAG_MODULE) {
            struct mb2_tag_module *bi_module = (struct mb2_tag_module *) bi_tag;
            if (ra_subtract_region(
                        (struct memregion) {.base = (u64) bi_module->mod_start,
                                            .size = ((u64) bi_module->mod_end - (u64) bi_module->mod_start)}) != 0) {
                return 1;
            }
        }
        bi_tag = (struct mb2_tag *) ((char *) bi_tag + ALIGN_UP(bi_tag->size, 8));
    }

    return 0;
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
