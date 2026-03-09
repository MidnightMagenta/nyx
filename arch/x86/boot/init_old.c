#include "multiboot2.h"
#include <asi/page.h>
#include <nyx/linkage.h>
#include <nyx/types.h>
#include <nyx/util.h>
#include <stddef.h>

#define hcf()                                                                                                          \
    while (1) { __asm__ volatile("hlt"); }

static void *__boot boot_memset(void *p, int v, size_t num) {
    u8 *strPtr = (u8 *) p;
    while (num--) { *strPtr++ = (u8) v; }
    return p;
}

#define BOOT_MAX_RESERVED_REGIONS 128

struct memregion {
    u64 base; // inclusive
    u64 end;  // exclusive
};

static struct memregion __bootzero reserved_regions[BOOT_MAX_RESERVED_REGIONS];
static u32 __bootdata              reserved_region_count = 0;

static void __boot reserve_region(u64 base, u64 end) {
    if (reserved_region_count >= BOOT_MAX_RESERVED_REGIONS) { hcf(); }
    reserved_regions[reserved_region_count++] = (struct memregion) {base, end};
}

extern char __image_phys_start;
extern char __image_phys_end;

static void *__bootdata image_phys_start = &__image_phys_start;
static void *__bootdata image_phys_end   = &__image_phys_end;

static void __boot reserve_boot_regions(u64 bi) {
    // reserve kernel image region
    reserve_region(ALIGN_DOWN((u64) image_phys_start, PAGE_SIZE), ALIGN_UP((u64) image_phys_end, PAGE_SIZE));

    // reserve boot info region
    u32 bi_size = *(u32 *) bi;
    reserve_region(ALIGN_DOWN(bi, PAGE_SIZE), ALIGN_UP(bi + bi_size, PAGE_SIZE));
}

static struct memregion __boot subtract_region(struct memregion a, struct memregion b) {
    struct memregion res = {0};

    // check both regions are sane
    if (a.end <= a.base || b.end <= b.base) { return (struct memregion) {0}; }

    // case 0: regions fully overlap
    if (a.base == b.base && a.end == b.end) { return (struct memregion) {0}; }

    // case 1: regions don't overlap
    if (b.end <= a.base || b.base >= a.end) {
        res = a;
        return res;
    }

    // case 2: region b overlaps region a at the start
    if (b.end > a.base && b.base <= a.base) {
        res.base = a.base + (b.end - a.base);
        res.end  = a.end;
        return res;
    }

    // case 3: region b overlaps region a at the end
    if (b.base < a.end && b.end >= a.end) {
        res.base = a.base;
        res.end  = a.end - (a.end - b.base);
        return res;
    }

    // case 4: region b is fully inside region a (return the larger of the two non overlapping regions)
    if (b.base > a.base && b.end < a.end) {
        struct memregion lower = {a.base, b.base};
        struct memregion upper = {b.end, a.end};
        if (lower.end - lower.base > upper.end - upper.base) {
            return lower;
        } else {
            return upper;
        }
    }

    return (struct memregion) {0};
}

static u64 __bootdata boot_alloc_base = 0;
static u64 __bootdata boot_alloc_top  = 0;


// FIXME: GRUB apparently doesn't mark it's own boot info as reserved memory
// this function should check if it won't encroach on GRUB's boot info
void __boot boot_alloc_init(u64 bi) {
    boot_alloc_base = 0;
    boot_alloc_top  = 0;

    boot_memset(reserved_regions, 0, BOOT_MAX_RESERVED_REGIONS * sizeof(struct memregion));
    reserve_boot_regions(bi);

    u32             bi_total_size = *(u32 *) bi;
    struct mb2_tag *bi_tag        = (struct mb2_tag *) (bi + 8);
    struct mb2_tag *bi_end        = (struct mb2_tag *) ((char *) bi_tag + bi_total_size);

    while (bi_tag->type != MB2_TAG_END && bi_tag <= bi_end) {
        if (bi_tag->type != MB2_TAG_MMAP) { goto next_tag; } // ignore non mmap tags

        struct mb2_tag_mmap *mmap_tag = (struct mb2_tag_mmap *) bi_tag;
        if (mmap_tag->entry_version != 0) { hcf(); } // unknown mmap entry version

        for (u8 *p = (u8 *) mmap_tag->entries; p < (u8 *) mmap_tag + mmap_tag->size; p += mmap_tag->entry_size) {
            struct mb2_mmap_entry *mmap_entry = (struct mb2_mmap_entry *) p;
            if (mmap_entry->type != MB2_MMAP_AVAILABLE) { continue; }

            u64 region_start = mmap_entry->addr;
            u64 region_end   = mmap_entry->addr + mmap_entry->len;
            if (region_end - region_start < 4 * UNIT_MiB) { continue; }

            // region is larger than 4M
            struct memregion region = {region_start, region_end};
            for (u32 i = 0; i < reserved_region_count; ++i) { region = subtract_region(region, reserved_regions[i]); }

            if (region.end - region.base > 4 * UNIT_MiB) {
                boot_alloc_base = ALIGN_UP(region.base, PAGE_SIZE);
                boot_alloc_top  = ALIGN_DOWN(region.end, PAGE_SIZE);
                return;
            }
        }

    next_tag:
        bi_tag = (struct mb2_tag *) ((char *) bi_tag + ALIGN_UP(bi_tag->size, 8));
    }

    hcf();
}

void *__boot boot_alloc_page() {
    if (!boot_alloc_base || !boot_alloc_top) { return NULL; }
    if (boot_alloc_base + PAGE_SIZE > boot_alloc_top) { return NULL; }

    u64 allocation = boot_alloc_base;
    boot_alloc_base += PAGE_SIZE;
    return (void *) allocation;
}

#define PML4_IDX(a) ((a >> 39) & 0x1FF)
#define PDP_IDX(a)  ((a >> 30) & 0x1FF)
#define PD_IDX(a)   ((a >> 21) & 0x1FF)
#define PT_IDX(a)   ((a >> 12) & 0x1FF)

// kernel image section markers
extern char __kernel_phys_start;
extern char __kernel_phys_end;
extern char __init_phys_start;
extern char __init_phys_end;

extern char __kernel_virt_start;
extern char __kernel_virt_end;
extern char __init_virt_start;
extern char __init_virt_end;

static void *__bootdata kernel_phys_start = &__kernel_phys_start;
static void *__bootdata kernel_phys_end   = &__kernel_phys_end;
static void *__bootdata init_phys_start   = &__init_phys_start;
static void *__bootdata init_phys_end     = &__init_phys_end;

static void *__bootdata kernel_virt_start = &__kernel_virt_start;
// static void *__bootdata kernel_virt_end   = &__kernel_virt_end;
static void *__bootdata init_virt_start = &__init_virt_start;
// static void *__bootdata init_virt_end     = &__init_virt_end;

// TODO: use large pages here if the address is 2M aligned
int __boot map_page_4k(u64 *pml4, u64 phys, u64 virt) {
    if (!pml4) { return 1; }
    if (phys & 0xFFF || virt & 0xFFF) { return 1; }

    if (!(pml4[PML4_IDX(virt)] & (1 << 0))) {
        // allocate a page
        u64 page = (u64) boot_alloc_page();
        boot_memset((void *) page, 0, 0x1000);
        pml4[PML4_IDX(virt)] = page | 0x3;
    }

    u64 *pdpt = (u64 *) (pml4[PML4_IDX(virt)] & ~0xFFFULL);

    if (!(pdpt[PDP_IDX(virt)] & (1 << 0))) {
        // allocate a pdpt
        u64 page = (u64) boot_alloc_page();
        boot_memset((void *) page, 0, 0x1000);
        pdpt[PDP_IDX(virt)] = page | 0x3;
    }

    u64 *pd = (u64 *) (pdpt[PDP_IDX(virt)] & ~0xFFFULL);

    if (!(pd[PD_IDX(virt)] & (1 << 0))) {
        // allocate pd
        u64 page = (u64) boot_alloc_page();
        boot_memset((void *) page, 0, 0x1000);
        pd[PD_IDX(virt)] = page | 0x3;
    }

    u64 *pt = (u64 *) (pd[PD_IDX(virt)] & ~0xFFFULL);
    if (pt[PT_IDX(virt)] & (1 << 0)) { return 1; }
    pt[PT_IDX(virt)] = (phys | 0x3ULL); // RW = 1, P = 1

    return 0;
}

void __boot boot_map_kernel(u64 *pml4) {
    u64 cur_phys = 0;
    u64 cur_virt = 0;

    if (!pml4) { hcf(); }

    // map init code
    cur_phys = (u64) init_phys_start;
    cur_virt = (u64) init_virt_start;
    while (cur_phys < (u64) init_phys_end) {
        if (map_page_4k(pml4, cur_phys, cur_virt) != 0) { hcf(); }
        cur_virt += 0x1000;
        cur_phys += 0x1000;
    }

    // map runtime code
    cur_phys = (u64) kernel_phys_start;
    cur_virt = (u64) kernel_virt_start;
    while (cur_phys < (u64) kernel_phys_end) {
        if (map_page_4k(pml4, cur_phys, cur_virt) != 0) { hcf(); }
        cur_virt += 0x1000;
        cur_phys += 0x1000;
    }
}
