#include <mm/memblock.h>
#include <mm/memory.h>
#include <mm/mm_types.h>
#include <mm/mmzone.h>
#include <nyx/align.h>
#include <nyx/limits.h>
#include <nyx/linkage.h>
#include <nyx/list.h>
#include <nyx/minmax.h>
#include <nyx/panic.h>
#include <nyx/string.h>
#include <nyx/types.h>

#include <asi/bootparam.h>
#include <asi/memory.h>
#include <asi/setupdata.h>

extern char __image_start;
extern char __image_end;

static inline phys_addr_t __init get_mem_bottom() {
    static phys_addr_t lowest = PHYS_ADDR_MAX;

    if (lowest != PHYS_ADDR_MAX) { return lowest; }

    for (size_t i = 0; i < bootparams->mmap_entry_count; ++i) {
        if (bootparams->mmap[i].type != MMAP_TYPE_AVAILABLE) { continue; }

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
        if (bootparams->mmap[i].type != MMAP_TYPE_AVAILABLE) { continue; }

        highest = MAX(highest, bootparams->mmap[i].addr + bootparams->mmap[i].size);
    }

    return highest;
}

static inline pfn_t __init get_highest_pfn() {
    return addr_to_pfn(ALIGN_UP(get_mem_top(), PAGE_SIZE));
}

extern pg_data_t contigmem_pagedata;

static u32 get_mem_type(pfn_t pfn) {
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

        if (pfn_to_addr(pfn) >= memmap_allocation && pfn_to_addr(pfn) <= (memmap_allocation + memmap_size)) {
            SetPageReserved(page);
            continue;
        }

        if (get_mem_type(pfn) != MMAP_TYPE_AVAILABLE) { SetPageReserved(page); }

        page->list = (struct list_head) LIST_HEAD_INIT(page->list);
    }
}

void __init init_zones() {
    // init zones
}
