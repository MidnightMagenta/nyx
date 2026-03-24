#include <mm/memblock.h>
#include <mm/memmap.h>
#include <mm/pmm.h>
#include <nyx/early_printk.h>
#include <nyx/errno.h>
#include <nyx/limits.h>
#include <nyx/linkage.h>
#include <nyx/panic.h>
#include <nyx/printk.h>
#include <nyx/types.h>
#include <nyx/util.h>

#define pr_fmt(fmt) "pmm: " fmt "\n"

struct page *page_map;

typedef u64       bm_word_t;
static bm_word_t *bm;
static size_t     bm_size; // in bits
static pfn_t      first_pfn;
static pfn_t      first_free_pfn_hint;
static pfn_t      last_free_pfn_hint;

#define BM_WORD_BITS  (sizeof(bm_word_t) * 8)
#define BM_WORD_SHIFT (__builtin_ctz(BM_WORD_BITS))
#define BM_WORD_MASK  (BM_WORD_BITS - 1)
#define BM_ONE        ((bm_word_t) 1)

#define PM_MAX_ALIGN PHYS_ADDR_MAX;

static size_t free;
static size_t reserved;
static size_t total;

static inline void bm_set(size_t bit);
static inline void bm_clear(size_t bit);
static inline int  bm_get(size_t bit);

static void __init get_memory_range(const struct memmap *memmap, phys_addr_t *loaddr, phys_addr_t *hiaddr) {
    *loaddr = U64_MAX;
    *hiaddr = 0;

    for (size_t i = 0; i < memmap->region_cnt; ++i) {
        const struct mem_region *r = &memmap->regions[i];
        early_printk(pr_fmt("mmap entry: type %d [0x%lx, 0x%lx]"), r->type, r->base, r->base + r->length);
        if (r->type != MEM_REGION_TYPE_AVAILABLE) { continue; }
        *loaddr = MIN(*loaddr, r->base);
        *hiaddr = MAX(*hiaddr, r->base + r->length);
    }

    *loaddr = ALIGN_UP(*loaddr, PAGE_SIZE);
    *hiaddr = ALIGN_DOWN(*hiaddr, PAGE_SIZE);
}

extern void pm_arch_reserve_regions();

static void __init setup_regions(const struct memmap *memmap) {
    int res;

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
                                 ALIGN_UP(total * sizeof(struct page), PAGE_SIZE) >> PAGE_SHIFT))) {
        early_panic(pr_fmt("could not reserve page map with %d"), res);
    }

    bm_set(0); // reserve PFN 0 as the NULL page
}

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


    alloc_size = page_count * sizeof(struct page);
    if ((res = memblock_aligned_alloc(&alloc_size, PAGE_SIZE, &alloc_addr))) {
        early_panic(pr_fmt("could not allocate page map with %d"), res);
    }

    page_map = (struct page *) alloc_addr;
    memset(page_map, 0, alloc_size);

    first_pfn           = addr_to_pfn(loaddr);
    first_free_pfn_hint = 0;
    last_free_pfn_hint  = bm_size;

    free     = 0;
    reserved = page_count;
    total    = page_count;

    setup_regions(memmap);
}

static inline void bm_set(size_t bit) {
    bm[bit >> BM_WORD_SHIFT] |= (BM_ONE << (bit & BM_WORD_MASK));
}

static inline void bm_clear(size_t bit) {
    bm[bit >> BM_WORD_SHIFT] &= ~(BM_ONE << (bit & BM_WORD_MASK));
}

static inline int bm_get(size_t bit) {
    return ((bm[bit >> BM_WORD_SHIFT] >> (bit & BM_WORD_MASK)) & BM_ONE);
}

static inline void reserve_page(pfn_t page) {
    bm_set(page);
    reserved++;
    free--;
}

static inline void free_page(pfn_t page) {
    bm_clear(page);
    reserved--;
    free++;
}

int pm_reserve_region(phys_addr_t addr, size_t count) {
    pfn_t pfn;

    if ((addr & (PAGE_SIZE - 1)) != 0) { return -EINVAL; }
    if (count == 0) { return 0; }

    pfn = addr_to_pfn(addr);

    if (pfn + count > bm_size) { return -ERANGE; }

    while (count--) {
        if (!bm_get(pfn)) {
            reserve_page(pfn);
        } else {
            pr_warn(pr_fmt("reserving an already reserved region at PFN %ld"), pfn);
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
            free_page(pfn);
        } else {
            pr_warn(pr_fmt("freeing an already free region at PFN %ld"), pfn);
        }
        pfn++;
    }

    return 0;
}

struct page *__pm_alloc_pages(u64 flags, u64 order) {
    size_t num_pages = (size_t) 1 << order;
    (void) flags;

    for (size_t i = 0; i < bm_size; i += num_pages) {
        bool found = true;
        for (size_t j = i; j < i + num_pages; j++) {
            if (bm_get(j)) {
                found = false;
                break;
            }
        }
        if (!found) { continue; }

        for (size_t j = i; j < i + num_pages; j++) { reserve_page(j); }

        struct page *head = &page_map[i];
        head->order       = order;
        head->flags |= PAGE_FLAG_HEAD;

        for (size_t j = 1; j < num_pages; j++) {
            page_map[i + j].order = order;
            page_map[i + j].flags |= PAGE_FLAG_TAIL;
        }

        return head;
    }

    return NULL;
}


void pm_free_pages(phys_addr_t addr) {
    size_t       num_pages;
    size_t       first_page = (addr >> PAGE_SHIFT) - first_pfn;
    struct page *head       = &page_map[first_page];

    if (!(head->flags & PAGE_FLAG_HEAD)) {
        pr_warn(pr_fmt("freeing a non-head page"));
        return;
    }

    num_pages = (size_t) 1 << head->order;

    for (size_t i = 1; i < num_pages; ++i) {
        struct page *tail = &page_map[first_page + i];
        if (!(tail->flags & PAGE_FLAG_TAIL)) { pr_warn(pr_fmt("freeing a non-tail page")); }
        tail->flags &= ~PAGE_FLAG_TAIL;
        tail->order = 0;
        free_page(first_page + i);
    }

    head->flags &= ~PAGE_FLAG_HEAD;
    head->order = 0;
    free_page(first_page);
}


phys_addr_t pm_page_to_phys(struct page *pg) {
    return ((pg - page_map) + first_pfn) << PAGE_SHIFT;
}

struct page *pm_phys_to_page(phys_addr_t addr) {
    return &page_map[(addr >> PAGE_SHIFT) - first_pfn];
}

int pm_is_free(phys_addr_t addr) {
    return !bm_get(addr_to_pfn(addr));
}

int pm_is_reserved(phys_addr_t addr) {
    return bm_get(addr_to_pfn(addr));
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
