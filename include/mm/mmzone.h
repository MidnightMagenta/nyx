#ifndef _MM_MMZONE_H
#define _MM_MMZONE_H

#include <mm/mm_types.h>
#include <nyx/bitmap.h>
#include <nyx/list.h>
#include <nyx/stddef.h>
#include <nyx/types.h>

#define MAX_ORDER 11

#define MIGRATE_TYPES 1

struct free_area_s {
    struct list_head list[MIGRATE_TYPES];
    DECLARE_DYNAMIC_BITMAP(buddy_bitmap);
    size_t free_count;
};

struct zone_s {
    size_t free_pages;
    // size_t pages_min, pages_low, pages_high;

    struct free_area_s free_area[MAX_ORDER];
    DECLARE_STATIC_BITMAP(free_bitmap, MAX_ORDER);

    struct page *zone_mem_map;
    size_t       zone_start_pfn;
    size_t       spanned_pages;
    size_t       present_pages;

    char *name;
};

#define MAX_ZONES 4

typedef struct pg_data_s {
    struct zone_s zones[MAX_ZONES];

    struct page *mem_map;

    phys_addr_t start_paddr;
    phys_addr_t end_paddr;
} pg_data_t;

#endif
