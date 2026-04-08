#include <mm/memblock.h>
#include <mm/memory.h>
#include <mm/mmzone.h>
#include <nyx/align.h>
#include <nyx/kernel.h>
#include <nyx/limits.h>
#include <nyx/linkage.h>
#include <nyx/list.h>
#include <nyx/minmax.h>
#include <nyx/panic.h>
#include <nyx/types.h>

#define pr_fmt(fmt)           "pmm: " fmt "\n"
#define buddy_bit(pfn, order) ((pfn >> order) >> 1)
#define norm_pfn(zone, pfn)   (pfn - zone->zone_start_pfn)

// TODO: correctness checks

static inline void list_bm_flip(zone_t *zone, pfn_t pfn, int order) {
    bm_flip(zone->free_area[order].buddy_bitmap, buddy_bit(pfn, order));
}

static inline bool list_bm_is_set(zone_t *zone, pfn_t pfn, int order) {
    return bm_get(zone->free_area[order].buddy_bitmap, buddy_bit(pfn, order));
}

static void list_add_block(zone_t *zone, pfn_t pfn, int order) {
    struct page *page = pfn_to_page(pfn);
    list_add(&page->list, &zone->free_area[order].list[0]);
}

static void list_rm_block(zone_t *zone, pfn_t pfn, int order) {
    (void) zone;
    (void) order;

    struct page *page = pfn_to_page(pfn);
    list_del(&page->list);
}

void __init init_zone_lists(zone_t *zone) {
    int         res;
    size_t      bitmap_size;
    phys_addr_t bitmap_addr;
    pfn_t       bitmap_pfn, bitmap_page_count;

    for (size_t i = 0; i < MAX_ORDER; ++i) {
        zone->free_area[i].list[0]    = (struct list_head) LIST_HEAD_INIT(zone->free_area[i].list[0]);
        zone->free_area[i].free_count = 0;

        bitmap_size = (zone->spanned_pages >> i) >> 1;
        bitmap_addr = PHYS_ADDR_MAX;

        if ((res = memblock_aligned_alloc(&bitmap_size, sizeof(bitmap_word_t), &bitmap_addr))) {
            early_panic(pr_fmt("could not allocate bitmap for oder %d with %d"), i, res);
        }

        bitmap_pfn        = addr_to_pfn(ALIGN_DOWN(bitmap_addr, PAGE_SIZE));
        bitmap_page_count = (ALIGN_UP(bitmap_size, PAGE_SIZE) >> PAGE_SHIFT);

        for (size_t i = 0; i < bitmap_page_count; ++i) { SetPageReserved(pfn_to_page(bitmap_pfn + i)); }

        zone->free_area[i].buddy_bitmap = (bitmap_word_t *) bitmap_addr;
    }
}

static inline __init bool any_reserved(struct page *page, pfn_t count) {
    for (pfn_t i = 0; i < count; ++i) {
        if (PageReserved(&page[i])) { return true; }
    }
    return false;
}

static inline size_t get_max_order(struct page *page, size_t max_pages) {
    pfn_t pfn   = page_to_pfn(page);
    int   order = MAX_ORDER;

    while (order > 0) {
        if ((pfn & ((1 << order) - 1)) == 0 && (1 << order) <= max_pages && !any_reserved(page, (1 << order))) {
            break;
        }

        order--;
    }

    return order;
}


void __init zone_insert_blocks(zone_t *zone) {
    size_t i = 0;

    while (i < zone->spanned_pages) {
        struct page *page  = &zone->zone_mem_map[i];
        int          order = get_max_order(page, MIN(zone->spanned_pages - i, (1 << MAX_ORDER)));

        if (PageReserved(page)) {
            i++;
            continue;
        }

        pr_dbg(pr_fmt("inserting %d order block at 0x%lx"), order, pfn_to_addr(page_to_pfn(page)));

        list_add_block(zone, page_to_pfn(page), order);
        list_bm_flip(zone, page_to_pfn(page), order);
        i += (1 << order);
    }
}

void __init init_page_alloc() {
    // TODO: this only will work for contigmem

    for (size_t i = 0; i < MAX_NR_ZONES; ++i) {
        zone_t *zone = &pgdata->zones[i];
        init_zone_lists(zone);
    }

    for (size_t i = 0; i < MAX_NR_ZONES; ++i) {
        zone_t *zone = &pgdata->zones[i];
        zone_insert_blocks(zone);
    }
}
