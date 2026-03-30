#include <mm/memblock.h>
#include <mm/memmap.h>
#include <mm/pmm.h>
#include <nyx/align.h>
#include <nyx/kernel.h>
#include <nyx/limits.h>
#include <nyx/linkage.h>
#include <nyx/panic.h>
#include <nyx/printk.h>
#include <nyx/types.h>

#define pr_fmt(fmt) "pmm: " fmt "\n"

struct page *page_map;

static void __init get_memory_range(const struct memmap *memmap, phys_addr_t *loaddr, phys_addr_t *hiaddr) {
    *loaddr = U64_MAX;
    *hiaddr = 0;

    for (size_t i = 0; i < memmap->region_cnt; ++i) {
        const struct mem_region *r = &memmap->regions[i];
        pr_dbg(pr_fmt("mmap entry: type %d [0x%lx, 0x%lx]"), r->type, r->base, r->base + r->length);
        if (r->type != MEM_REGION_TYPE_AVAILABLE) { continue; }
        *loaddr = MIN(*loaddr, r->base);
        *hiaddr = MAX(*hiaddr, r->base + r->length);
    }

    *loaddr = ALIGN_UP(*loaddr, PAGE_SIZE);
    *hiaddr = ALIGN_DOWN(*hiaddr, PAGE_SIZE);
}

extern void pm_arch_reserve_regions();

void __init __do_pm_init(const struct memmap *memmap) {}
