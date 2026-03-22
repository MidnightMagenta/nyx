#include <mm/memblock.h>
#include <mm/memmap.h>
#include <mm/pmm.h>
#include <nyx/early_printk.h>
#include <nyx/errno.h>
#include <nyx/limits.h>
#include <nyx/linkage.h>
#include <nyx/panic.h>
#include <nyx/types.h>
#include <nyx/util.h>

#define pr_fmt(fmt) ("pmm: " fmt "\n")

struct page *page_map;

typedef u64       bm_word_t;
static bm_word_t *bm;
static size_t     bm_size; // in bits

#define BM_WORD_BITS  (sizeof(bm_word_t) * 8)
#define BM_WORD_SHIFT (__builtin_ctz(BM_WORD_BITS))
#define BM_WORD_MASK  (BM_WORD_BITS - 1)
#define BM_ONE        ((bm_word_t) 1)

#define PM_MAX_ALIGN PHYS_ADDR_MAX;

static size_t free;
static size_t reserved;
static size_t total;

static void __init get_memory_range(const struct memmap *memmap, phys_addr_t *loaddr, phys_addr_t *hiaddr) {
    *loaddr = U64_MAX;
    *hiaddr = 0;

    for (size_t i = 0; i < memmap->region_cnt; ++i) {
        const struct mem_region *r = &memmap->regions[i];
        early_printk(pr_fmt("mmap entry %d: [0x%lx, 0x%lx]"), r->type, r->base, r->base + r->length);
        if (r->type != MEM_REGION_TYPE_AVAILABLE) { continue; }
        *loaddr = MIN(*loaddr, r->base);
        *hiaddr = MAX(*hiaddr, r->base + r->length);
    }

    *loaddr = ALIGN_UP(*loaddr, PAGE_SIZE);
    *hiaddr = ALIGN_DOWN(*hiaddr, PAGE_SIZE);
}

extern void pm_arch_reserve_regions();

void __init __do_pm_init(const struct memmap *memmap) {
    int         res;
    phys_addr_t loaddr, hiaddr;
    size_t      page_count;
    size_t      alloc_size;
    phys_addr_t alloc_addr;

    if (!memmap || !memmap->region_cnt) { early_panic(pr_fmt("Invalid memory map")); }

    get_memory_range(memmap, &loaddr, &hiaddr);

    page_count = (hiaddr - loaddr) / PAGE_SIZE;
    alloc_size = page_count / 8;

    if ((res = memblock_aligned_alloc(&alloc_size, PAGE_SIZE, &alloc_addr))) {
        early_panic(pr_fmt("could not allocate memory bitmap with %d"), res);
    }

    bm_size = page_count;

    bm = (bm_word_t *) alloc_addr;
    memset(bm, 0xFF, alloc_size);

    free     = 0;
    reserved = page_count;
    total    = page_count;

    alloc_size = page_count * sizeof(struct page);
    if ((res = memblock_aligned_alloc(&alloc_size, PAGE_SIZE, &alloc_addr))) {
        early_panic(pr_fmt("could not allocate page map with %d"), res);
    }

    page_map = (struct page *) alloc_addr;
    memset(page_map, 0, alloc_size);

    for (size_t i = 0; i < memmap->region_cnt; ++i) {
        const struct mem_region *r = &memmap->regions[i];
        if (r->type != MEM_REGION_TYPE_AVAILABLE) { continue; }

        phys_addr_t base = ALIGN_UP(r->base, PAGE_SIZE);
        phys_addr_t end  = ALIGN_DOWN(r->base + r->length, PAGE_SIZE);
        if (end > base) { pm_free_region(base, (end - base) >> PAGE_SHIFT); }
    }

    pm_arch_reserve_regions();

    if ((res = pm_reserve_region((phys_addr_t) bm, ALIGN_UP(bm_size / 8, PAGE_SIZE) >> PAGE_SHIFT))) {
        early_panic(pr_fmt("could not reserve bitmap with %d"), res);
    }

    if ((res = pm_reserve_region((phys_addr_t) page_map,
                                 ALIGN_UP(page_count * sizeof(struct page), PAGE_SIZE) >> PAGE_SHIFT))) {
        early_panic(pr_fmt("could not reserve page map with %d"), res);
    }
}

static void bm_set(size_t bit) {
    bm[bit >> BM_WORD_SHIFT] |= (BM_ONE << (bit & BM_WORD_MASK));
}

static void bm_clear(size_t bit) {
    bm[bit >> BM_WORD_SHIFT] &= ~(BM_ONE << (bit & BM_WORD_MASK));
}

static int bm_get(size_t bit) {
    return ((bm[bit >> BM_WORD_SHIFT] >> (bit & BM_WORD_MASK)) & BM_ONE);
}

int pm_reserve_region(phys_addr_t addr, size_t count) {
    pfn_t pfn;

    if ((addr & (PAGE_SIZE - 1)) != 0) { return -EINVAL; }
    if (count == 0) { return 0; }

    pfn = addr_to_pfn(addr);

    if (pfn + count > bm_size) { return -ERANGE; }

    while (count--) {
        if (!bm_get(pfn)) {
            bm_set(pfn);
            free--;
            reserved++;
        } else {
            // TODO: resplace with runtime printk()
            early_printk(pr_fmt("warning: reserving an already reserved region at PFN %ld"), pfn);
        }
        pfn++;
    }

    return 0;
}

int pm_free_region(phys_addr_t addr, size_t count) {
    pfn_t pfn;

    if ((addr & (PAGE_SIZE - 1)) != 0) { return -EINVAL; }
    if (count == 0) { return 0; }

    pfn = addr_to_pfn(addr);

    if (pfn + count > bm_size) { return -ERANGE; }

    while (count--) {
        if (bm_get(pfn)) {
            bm_clear(pfn);
            free++;
            reserved--;
        } else {
            // TODO: resplace with runtime printk()
            early_printk(pr_fmt("warning: freeing an already free region at PFN %ld"), pfn);
        }
        pfn++;
    }

    return 0;
}

size_t pm_getstat(enum pm_stat stat) {
    switch (stat) {
        case PM_STAT_MIN_ALIGN:
            return PAGE_SIZE;
        case PM_STAT_MAX_ALIGN:
            return PM_MAX_ALIGN;
        case PM_STAT_TOTAL:
            return total;
        case PM_STAT_FREE:
            return free;
        case PM_STAT_USED:
            return reserved;
        default:
            return 0;
    }
}
