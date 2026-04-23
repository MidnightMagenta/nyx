#ifndef _MM_MEMORY_H
#define _MM_MEMORY_H

#include <asi/memory.h>
#include <asi/page.h>

#include <mm/mm_types.h>
#include <mm/mmzone.h>

extern struct pg_data_s *pgdata;

static inline pfn_t page_to_pfn(struct page *page) {
    zone_t *zone = &pgdata->zones[page->zone_id];
    return (page - zone->zone_mem_map) + zone->zone_start_pfn;
}

static inline struct page *pfn_to_page(pfn_t pfn) {
    for (int i = 0; i < MAX_NR_ZONES; i++) {
        zone_t *zone = &pgdata->zones[i];
        if (pfn >= zone->zone_start_pfn && pfn < zone->zone_start_pfn + zone->spanned_pages) {
            return &zone->zone_mem_map[pfn - zone->zone_start_pfn];
        }
    }
    return NULL;
}

static inline struct page *phys_to_page(phys_addr_t phys) {
    return pfn_to_page(phys >> PAGE_SHIFT);
}

static inline phys_addr_t page_to_phys(struct page *page) {
    return (phys_addr_t) (page_to_pfn(page) << PAGE_SHIFT);
}

#define page_address(page) phys_to_virt((void *) page_to_phys((page)))
#define virt_to_page(virt) phys_to_page(virt_to_phys((phys_addr_t) (virt)))

#endif
