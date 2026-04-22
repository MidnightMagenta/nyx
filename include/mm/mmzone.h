#ifndef _MM_MMZONE_H
#define _MM_MMZONE_H

#include <asi/page.h>
#include <mm/mm_types.h>
#include <nyx/list.h>
#include <nyx/stddef.h>
#include <nyx/types.h>

/* represents the count of per order free lists, not the
 * maximum allocable block order */
#define MAX_ORDER 11

enum migrate_types {
    MIGRATE_NONE = 0,
    NR_MIGRATE_TYPES,
};

struct free_area_s {
    /* lists are indexed by values of enum migrate_types */
    struct list_head list[NR_MIGRATE_TYPES];
    size_t           free_count;
};

typedef struct zone_s {
    size_t free_pages;
    // TODO: watermarks

    /* index into this array is block order */
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
    MAX_NR_ZONES,
};

struct zonelist {
    zone_t *zones[MAX_NR_ZONES + 1]; // null terminated
};

typedef struct pg_data_s {
    /* zones and zonelists are indexed by values of enum zone_type */
    struct zone_s   zones[MAX_NR_ZONES];
    struct zonelist zonelists[MAX_NR_ZONES];

    pfn_t  start_pfn;
    size_t spanned_pages;
    size_t present_pages;
} pg_data_t;

#endif
