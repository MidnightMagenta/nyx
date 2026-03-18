#ifndef _MM_MEMBLOCK_H
#define _MM_MEMBLOCK_H

#include <nyx/types.h>
#include <stddef.h>

struct memblock_region {
    phys_addr_t base;
    phys_addr_t size;
};

struct memblock_type {
    size_t                  cnt;
    size_t                  max;
    phys_addr_t             total_size;
    struct memblock_region *regions;
};

struct memblock {
    bool                 bottom_up;
    struct memblock_type memory;
    struct memblock_type reserved;
};

extern struct memblock memblock;

enum {
    MEMBLOCK_STAT_MEMSZ,
    MEMBLOCK_STAT_RESSZ,
    MEMBLOCK_STAT_MEM_REGION_CNT,
    MEMBLOCK_STAT_RES_REGION_CNT,
};

void memblock_init();
int  memblock_add_memory(phys_addr_t addr, size_t size);
int  memblock_delete_memory(phys_addr_t addr, size_t size);
int  memblock_reserve(phys_addr_t addr, size_t size);
int  memblock_unreserve(phys_addr_t addr, size_t size);
int  memblock_is_reserved(phys_addr_t addr, size_t size);
int  memblock_is_memory(phys_addr_t addr, size_t size);
int  memblock_alloc(size_t *size, phys_addr_t *out);
int  memblock_aligned_alloc(size_t *size, size_t alignment, phys_addr_t *out);
int  memblock_free_all();
u64  memblock_getstat(int stat);

#endif
