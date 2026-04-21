#ifndef _MM_MMZONE_H
#define _MM_MMZONE_H

#include <asi/page.h>
#include <mm/mm_types.h>
#include <nyx/list.h>
#include <nyx/stddef.h>
#include <nyx/types.h>

#define MAX_ORDER 11

#define MIGRATE_TYPES 1

struct free_area_s {
    struct list_head list[MIGRATE_TYPES];
    size_t           free_count;
};

typedef struct zone_s {
    size_t free_pages;
    // size_t pages_min, pages_low, pages_high;

    struct free_area_s free_area[MAX_ORDER];

    struct page *zone_mem_map;
    size_t       zone_start_pfn;
    size_t       spanned_pages;
    size_t       present_pages;

    char *name;
} zone_t;

enum zone_type {
#ifdef CONFIG_ZONE_DMA
    ZONE_DMA,
#endif
#ifdef CONFIG_ZONE_DMA32
    ZONE_DMA32,
#endif
    ZONE_NORMAL,
#ifdef CONFIG_ZONE_HIGHMEM
    ZONE_HIGHMEM,
#endif
    __MAX_NR_ZONES,
};

#define MAX_NR_ZONES __MAX_NR_ZONES

struct zonelist {
    zone_t *zones[MAX_NR_ZONES + 1];
};

typedef struct pg_data_s {
    struct zone_s   zones[MAX_NR_ZONES];
    struct zonelist zonelists[MAX_NR_ZONES];

    struct page *mem_map;

    pfn_t start_pfn;
    pfn_t end_pfn;
} pg_data_t;

#endif
