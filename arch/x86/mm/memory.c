#include <mm/memblock.h>
#include <mm/memory.h>
#include <mm/mm_types.h>
#include <mm/mmzone.h>
#include <nyx/align.h>
#include <nyx/kernel.h>
#include <nyx/limits.h>
#include <nyx/linkage.h>
#include <nyx/list.h>
#include <nyx/mem_units.h>
#include <nyx/minmax.h>
#include <nyx/panic.h>
#include <nyx/string.h>
#include <nyx/types.h>

#include <asi/bootparam.h>
#include <asi/memory.h>
#include <asi/setupdata.h>

#define pr_fmt(fmt) "memory: " fmt "\n"

extern char __image_start;
extern char __image_end;

static inline void __init pr_mmap() {
    for (size_t i = 0; i < bootparams->mmap_entry_count; ++i) {
        pr_info("memmap: [0x%lx 0x%lx] - %s\n",
                bootparams->mmap[i].addr,
                bootparams->mmap[i].addr + bootparams->mmap[i].size,
                mmap_type_string(&bootparams->mmap[i]));
    }
}

static inline phys_addr_t __init get_mem_bottom() {
    static phys_addr_t lowest = PHYS_ADDR_MAX;

    if (lowest != PHYS_ADDR_MAX) { return lowest; }

    for (size_t i = 0; i < bootparams->mmap_entry_count; ++i) {
        if (!mmap_is_memory(&bootparams->mmap[i])) { continue; }

        lowest = MIN(lowest, bootparams->mmap[i].addr);
    }

    return lowest;
}

static inline pfn_t __init get_lowest_pfn() {
    return addr_to_pfn(ALIGN_DOWN(get_mem_bottom(), PAGE_SIZE));
}

static inline phys_addr_t __init get_mem_top() {
    static phys_addr_t highest = 0;

    if (highest != 0) { return highest; }

    for (size_t i = 0; i < bootparams->mmap_entry_count; ++i) {
        if (!mmap_is_memory(&bootparams->mmap[i])) { continue; }

        highest = MAX(highest, bootparams->mmap[i].addr + bootparams->mmap[i].size);
    }

    return highest;
}

static inline pfn_t __init get_highest_pfn() {
    return addr_to_pfn(ALIGN_UP(get_mem_top(), PAGE_SIZE));
}

extern pg_data_t contigmem_pagedata;

struct {
    phys_addr_t base;
    phys_addr_t top;
    char       *name;
} zones[MAX_NR_ZONES] __initdata = {
        [ZONE_DMA]    = {0, 16 * MiB, "ZONE_DMA"},
        [ZONE_DMA32]  = {16 * MiB, 4 * GiB, "ZONE_DMA32"},
        [ZONE_NORMAL] = {4 * GiB, PHYS_ADDR_MAX, "ZONE_NORMAL"},
};

static void __init get_mem_limits_within_range(phys_addr_t *loaddr, phys_addr_t *hiaddr) {
    phys_addr_t low = PHYS_ADDR_MAX, high = 0;

    for (size_t i = 0; i < bootparams->mmap_entry_count; ++i) {
        struct mmap_entry *ent = &bootparams->mmap[i];
        phys_addr_t        start, end;

        if (!mmap_is_memory(ent)) { continue; }

        start = ent->addr;
        end   = ent->addr + ent->size;

        if (*loaddr > end || *hiaddr < start) { continue; }

        // clang-format off
        if (start >= *loaddr || start <= *hiaddr) { 
            low = MIN(low, MAX(start, *loaddr)); 
        }
        if (end >= *loaddr || start <= *hiaddr) { 
            high = MAX(high, MIN(end, *hiaddr)); 
        }
        // clang-format on
    }

    if (low > high) {
        *loaddr = 0;
        *hiaddr = 0;
        return;
    }

    *loaddr = low;
    *hiaddr = high;
}

static void __init fixup_zones() {
    for (size_t i = 0; i < ARRAY_SIZE(zones); ++i) { get_mem_limits_within_range(&zones[i].base, &zones[i].top); }
}

static pfn_t __init get_page_count_for_range(pfn_t start_pfn, pfn_t end_pfn) {
    pfn_t page_cnt = 0;

    for (size_t i = start_pfn; i < end_pfn && i < contigmem_pagedata.end_pfn; ++i) {
        if (PageReserved(&contigmem_pagedata.mem_map[i])) { continue; }
        page_cnt++;
    }

    return page_cnt;
}

static void __init init_zones() {
    fixup_zones();

    for (size_t i = 0; i < ARRAY_SIZE(zones); ++i) {
        struct zone_s *zone = &contigmem_pagedata.zones[i];
        memset(zone, 0, sizeof(struct zone_s));

        zone->zone_mem_map   = pfn_to_page(addr_to_pfn(zones[i].base));
        zone->zone_start_pfn = addr_to_pfn(zones[i].base);
        zone->spanned_pages  = (zones[i].top - zones[i].base) >> PAGE_SHIFT;
        zone->present_pages  = get_page_count_for_range(addr_to_pfn(zones[i].base), addr_to_pfn(zones[i].top));
        zone->name           = zones[i].name;
    }

#ifdef __DEBUG
    for (size_t i = 0; i < MAX_NR_ZONES; ++i) {
        pr_dbg("zone %s:\n  "
               "mem_map addr:  0x%lx\n  "
               "start pfn:     %ld\n  "
               "spanned pages: %ld\n  "
               "present pages: %ld\n",
               contigmem_pagedata.zones[i].name,
               contigmem_pagedata.zones[i].zone_mem_map,
               contigmem_pagedata.zones[i].zone_start_pfn,
               contigmem_pagedata.zones[i].spanned_pages,
               contigmem_pagedata.zones[i].present_pages);
    }
#endif
}

static u32 __init get_mem_type(pfn_t pfn) {
    phys_addr_t addr = pfn_to_addr(pfn);

    if (addr >= bootparams->kernel_load_base &&
        (addr + PAGE_SIZE) <= (bootparams->kernel_load_base + (phys_addr_t) (&__image_end - &__image_start))) {
        return MMAP_TYPE_RESERVED;
    }

    for (size_t i = 0; i < bootparams->mmap_entry_count; ++i) {
        struct mmap_entry *ent = &bootparams->mmap[i];

        if (addr >= ent->addr && (addr + PAGE_SIZE) <= (ent->addr + ent->size)) { return ent->type; }
    }

    return MMAP_TYPE_NONE;
}

void __init init_memmap() {
    int         res;
    pfn_t       lopfn, hipfn;
    phys_addr_t memmap_allocation;
    size_t      memmap_size;

    pr_mmap();

    lopfn = get_lowest_pfn();
    hipfn = get_highest_pfn();

    memmap_size = (hipfn - lopfn) * sizeof(struct page);

    if ((res = memblock_alloc(&memmap_size, &memmap_allocation)) ||
        memmap_size < ((hipfn - lopfn) * sizeof(struct page))) {
        early_panic("could not allocate mem map: %d", res);
    }

    contigmem_pagedata.mem_map   = (struct page *) phys_to_virt(memmap_allocation);
    contigmem_pagedata.start_pfn = lopfn;
    contigmem_pagedata.end_pfn   = hipfn;

    for (size_t i = 0; i < (hipfn - lopfn); ++i) {
        struct page *page = &contigmem_pagedata.mem_map[i];
        pfn_t        pfn  = i + contigmem_pagedata.start_pfn;

        memset(page, 0, sizeof(struct page));
        page->list = (struct list_head) LIST_HEAD_INIT(page->list);
        SetPageReserved(page);

        if (memblock_is_reserved(pfn_to_addr(pfn), PAGE_SIZE)) { continue; }

        if (get_mem_type(pfn) == MMAP_TYPE_AVAILABLE) { ClearPageReserved(page); }
    }

    init_zones();
}
