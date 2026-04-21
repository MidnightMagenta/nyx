#include <asi/bug.h>
#include <mm/memblock.h>
#include <mm/memory.h>
#include <mm/mmzone.h>
#include <mm/physmem.h>
#include <nyx/align.h>
#include <nyx/kernel.h>
#include <nyx/limits.h>
#include <nyx/linkage.h>
#include <nyx/list.h>
#include <nyx/minmax.h>
#include <nyx/panic.h>
#include <nyx/types.h>

#define pr_fmt(fmt) "pmm: " fmt

#ifdef CONFIG_PHYSMEM_DEV_PRINT
#define physmem_pr_dev(fmt, ...) printk(pr_fmt(fmt), ##__VA_ARGS__)
#else
#define physmem_pr_dev(fmt, ...) /* void */
#endif


void __init init_page_alloc() { /* void */ }

#define buddy_of(addr, order) ((addr) ^ (1ul << (order)))

static inline int page_is_buddy(struct page *page, int order) {
    if (!PageBuddy(page)) { return 0; }
    return page->buddy_order == order;
}

static struct page *__rm_block(zone_t *zone, int order) {
    struct free_area_s *area;
    int                 cur_order;
    struct page        *page, *buddy;
    size_t              size;

    for (cur_order = order; cur_order < MAX_ORDER; ++cur_order) {
        area = &zone->free_area[cur_order];
        if (!list_is_empty(&area->list[0])) { goto block_found; }
    }
    return NULL;

block_found:
    page = list_first_entry(&area->list[0], struct page, list);
    list_del(&page->list);
    ClearPageBuddy(page);
    page->buddy_order = 0;
    area->free_count--;

    size = 1ul << cur_order;
    while (cur_order > order) {
        cur_order--;
        area = &zone->free_area[cur_order];
        size >>= 1;
        buddy = page + size;
        list_add(&buddy->list, &area->list[0]);
        area->free_count++;
        SetPageBuddy(buddy);
        buddy->buddy_order = cur_order;
    }

    zone->free_pages -= 1ul << order;
    return page;
}

static void __add_block(struct page *page, zone_t *zone, int order) {
    struct page *base     = zone->zone_mem_map;
    size_t       page_idx = page - base;
    size_t       buddy_idx;
    struct page *buddy;

    zone->free_pages += 1ul << order;

    while (order < MAX_ORDER - 1) {
        buddy_idx = buddy_of(page_idx, order);
        buddy     = base + buddy_idx;

        if (!page_is_buddy(buddy, order)) { break; }

        list_del(&buddy->list);
        ClearPageBuddy(buddy);

        zone->free_area[order].free_count--;

        page_idx &= ~(1ul << order);
        page = base + page_idx;
        order++;
    }

    list_add(&page->list, &zone->free_area[order].list[0]);
    SetPageBuddy(page);
    page->buddy_order = order;
    zone->free_area[order].free_count++;
}

static inline int gfp_zonelist(int gfp_mask) {
    if (gfp_mask & __GFP_DMA) { return ZONE_DMA; }
    if (gfp_mask & __GFP_DMA32) { return ZONE_DMA32; }

#ifdef CONFIG_ZONE_HIGHMEM
    if (gfp_mask & __GFP_HIMEM) { return ZONE_HIMEM; }
#endif

    return ZONE_NORMAL;
}

struct page *pm_alloc_pages(int gfp_mask, int order) {
    const struct zonelist *zlist;
    struct page           *page;
    zone_t                *zone;

    zlist = &pgdata->zonelists[gfp_zonelist(gfp_mask)];
    for (int i = 0; zlist->zones[i] != NULL; ++i) {
        zone = zlist->zones[i];
        page = __rm_block(zone, order);
        if (page) {
            physmem_pr_dev("allocating block of order %d at %#p\n", order, page_to_phys(page));
            return page;
        }
    }

    return NULL;
}

phys_addr_t __pm_get_free_pages(int gfp_mask, int order) {
    struct page *page = pm_alloc_pages(gfp_mask, order);
    if (page) { return page_to_phys(page); }

    return INVALID_PHYS_ADDR;
}

void __pm_free_pages(struct page *page, int order) {
    physmem_pr_dev("freeing block at %#p at order %d\n", page_to_phys(page), order);
    zone_t *zone = &pgdata->zones[page->zone];
    __add_block(page, zone, order);
}

void pm_free_pages(phys_addr_t addr, int order) {
    struct page *page = phys_to_page(addr);
    __pm_free_pages(page, order);
}
