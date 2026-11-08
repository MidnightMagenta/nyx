#ifndef _MM_MEMBLOCK_H
#define _MM_MEMBLOCK_H

#include <nyx/stddef.h>
#include <nyx/types.h>

struct memblock_region {
    phys_addr_t base;
    phys_addr_t size;
};

struct memblock_type {
    size_t                  cnt;
    size_t                  max;
    phys_addr_t             total_size;
    struct memblock_region *regions;
    const char             *name;
};

struct memblock {
    struct memblock_type memory;
    struct memblock_type reserved;
};

extern struct memblock memblock;

void        memblock_init();
int         memblock_add(phys_addr_t addr, size_t size);
int         memblock_remove(phys_addr_t addr, size_t size);
void        memblock_trim();
void        memblock_reserve(phys_addr_t addr, size_t size);
int         memblock_is_any_reserved(phys_addr_t addr, size_t size);
int         memblock_is_memory(phys_addr_t addr, size_t size);
phys_addr_t memblock_alloc(size_t size);
phys_addr_t memblock_aligned_alloc(size_t size, size_t alignment);
void        memblock_free(phys_addr_t addr, u64 size);
void        memblock_free_all();

#ifdef __DEBUG
void memblock_print_regions();
#else
#define memblock_print_regions() /* void */
#endif

#endif
