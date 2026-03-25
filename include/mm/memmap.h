#ifndef _MM_MEMMAP_H
#define _MM_MEMMAP_H

#include <nyx/stddef.h>
#include <nyx/types.h>

#define MEMMAP_MAX_REGIONS 128

#define MEM_REGION_TYPE_NONE             0
#define MEM_REGION_TYPE_AVAILABLE        1
#define MEM_REGION_TYPE_RESERVED         2
#define MEM_REGION_TYPE_UNUSABLE         3
#define MEM_REGION_TYPE_BOOT_RECLAIMABLE 4

struct mem_region {
    u32         type;
    phys_addr_t base;
    phys_addr_t length;
};

struct memmap {
    struct mem_region regions[MEMMAP_MAX_REGIONS];
    size_t            region_cnt;
};

#endif
