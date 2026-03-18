#include <mm/memblock.h>
#include <nyx/errno.h>
#include <nyx/linkage.h>
#include <nyx/util.h>
#include <string.h>

#define MEMBLOCK_INIT_REGIONS 128

static struct memblock_region memblock_memory_init_regions[MEMBLOCK_INIT_REGIONS] __initdata;
static struct memblock_region memblock_reserved_init_regions[MEMBLOCK_INIT_REGIONS] __initdata;

struct memblock memblock __initdata = {
        .memory.regions = memblock_memory_init_regions,
        .memory.cnt     = 0,
        .memory.max     = MEMBLOCK_INIT_REGIONS,

        .reserved.regions = memblock_reserved_init_regions,
        .reserved.cnt     = 0,
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

#define memblock_sort_and_merge(r, n)                                                                                  \
    memblock_sort(r, n);                                                                                               \
    memblock_merge_regions(r, n);

static int memblock_add_region(struct memblock_type *regions, phys_addr_t addr, size_t size) {
    if (regions->cnt >= regions->max) { return -ENOSPC; }
    regions->regions[regions->cnt++] = (struct memblock_region) {addr, size};
    memblock_sort_and_merge(regions->regions, &regions->cnt);
    regions->total_size += size;
    return 0;
}

static int memblock_subtract_region(struct memblock_type *regions, phys_addr_t addr, size_t size) {
    u64    sb = addr;
    u64    se;
    size_t i;

    /* Overflow check */
    if (size == 0) return 0;

    se = sb + size;
    if (se < sb) return -EINVAL;

    for (i = 0; i < regions->cnt; i++) {
        struct memblock_region *r  = &regions->regions[i];
        u64                     rb = r->base;
        u64                     re = r->base + r->size;

        /* Case 0: No overlap */
        if (se <= rb || sb >= re) continue;

        /* Case 1: remove entire region */
        if (sb <= rb && se >= re) {
            regions->total_size -= r->size;

            memmove(&regions->regions[i], &regions->regions[i + 1], (regions->cnt - i - 1) * sizeof(*r));

            regions->cnt--;
            i--;
            continue;
        }

        /* Case 2: trim from left */
        if (sb <= rb) {
            phys_addr_t new_base = se;
            phys_addr_t new_size = re - se;

            regions->total_size -= (new_base - rb);

            r->base = new_base;
            r->size = new_size;
            continue;
        }

        /* Case 3: trim from right */
        if (se >= re) {
            phys_addr_t removed = re - sb;

            regions->total_size -= removed;

            r->size = sb - rb;
            continue;
        }

        /* Case 4: split region */
        if (regions->cnt >= regions->max) return -ENOMEM;

        memmove(&regions->regions[i + 2], &regions->regions[i + 1], (regions->cnt - i - 1) * sizeof(*r));

        regions->regions[i + 1].base = se;
        regions->regions[i + 1].size = re - se;

        r->size = sb - rb;

        regions->total_size -= (se - sb);

        regions->cnt++;

        break;
    }

    return 0;
}

int memblock_add_memory(phys_addr_t addr, size_t size) {
    return memblock_add_region(&memblock.memory, addr, size);
}

int memblock_delete_memory(phys_addr_t addr, size_t size) {
    return memblock_subtract_region(&memblock.memory, addr, size);
}

int memblock_reserve(phys_addr_t addr, size_t size) {
    return memblock_add_region(&memblock.reserved, addr, size);
}

int memblock_unreserve(phys_addr_t addr, size_t size) {
    return memblock_subtract_region(&memblock.reserved, addr, size);
}

int memblock_is_reserved(phys_addr_t addr, size_t size) {
    for (size_t i = 0; i < memblock.reserved.cnt; ++i) {
        struct memblock_region *r = &memblock.reserved.regions[i];

        u64 rb  = r->base;
        u64 re  = r->base + r->size;
        u64 end = addr + size;

        if (addr >= rb && end <= re) return 1;
    }

    return 0;
}

int memblock_is_memory(phys_addr_t addr, size_t size) {
    for (size_t i = 0; i < memblock.memory.cnt; ++i) {
        struct memblock_region *r = &memblock.memory.regions[i];

        u64 rb  = r->base;
        u64 re  = r->base + r->size;
        u64 end = addr + size;

        if (addr >= rb && end <= re) return 1;
    }

    return 0;
}

static int memblock_alloc_bottom_up(size_t *size, size_t alignment, phys_addr_t *out) {
    size_t sz = ALIGN_UP(*size, alignment);

    struct memblock_type *mem = &memblock.memory;

    for (size_t i = 0; i < mem->cnt; i++) {
        struct memblock_region *r = &mem->regions[i];

        phys_addr_t start = r->base;
        phys_addr_t end   = r->base + r->size;

        phys_addr_t cand = ALIGN_UP(start, alignment);

        while (cand + sz <= end) {
            if (memblock_is_memory(cand, sz) && !memblock_is_reserved(cand, sz)) {

                memblock_reserve(cand, sz);

                *out  = cand;
                *size = sz;
                return 0;
            }

            cand += alignment;
        }
    }

    return -ENOMEM;
}

static int memblock_alloc_top_down(size_t *size, size_t alignment, phys_addr_t *out) {
    size_t sz = ALIGN_UP(*size, alignment);

    struct memblock_type *mem = &memblock.memory;

    for (ssize_t i = mem->cnt - 1; i >= 0; i--) {
        struct memblock_region *r = &mem->regions[i];

        phys_addr_t start = r->base;
        phys_addr_t end   = r->base + r->size;

        phys_addr_t cand = ALIGN_DOWN((end - sz), alignment);

        while (cand >= start) {
            if (memblock_is_memory(cand, sz) && !memblock_is_reserved(cand, sz)) {
                memblock_reserve(cand, sz);

                *out  = cand;
                *size = sz;
                return 0;
            }

            if (cand < alignment) break;
            cand -= alignment;
        }
    }

    return -ENOMEM;
}

int memblock_alloc(size_t *size, phys_addr_t *out) {
    return memblock_aligned_alloc(size, 1, out);
}

int memblock_aligned_alloc(size_t *size, size_t alignment, phys_addr_t *out) {
    if (!size || !out) { return -EINVAL; }
    if (*size == 0 || alignment == 0) { return -EINVAL; }

    if (memblock.bottom_up) { return memblock_alloc_bottom_up(size, alignment, out); }
    return memblock_alloc_top_down(size, alignment, out);
}

u64 memblock_getstat(int stat) {
    switch (stat) {
        case MEMBLOCK_STAT_MEMSZ:
            return memblock.memory.total_size;
        case MEMBLOCK_STAT_RESSZ:
            return memblock.reserved.total_size;
        case MEMBLOCK_STAT_MEM_REGION_CNT:
            return memblock.memory.cnt;
        case MEMBLOCK_STAT_RES_REGION_CNT:
            return memblock.reserved.cnt;
        default:
            return 0;
    }
}
