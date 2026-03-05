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

int   memblock_reserve_region(phys_addr_t addr, size_t size);
int   memblock_unreserve_region(phys_addr_t addr, size_t size);
int   memblock_init(phys_addr_t mmap);
void *memblock_alloc(size_t size);
void *memblock_aligned_alloc(size_t size, size_t alignment);
void  memblock_free(void *mem);
int   memblock_free_all();


#endif
