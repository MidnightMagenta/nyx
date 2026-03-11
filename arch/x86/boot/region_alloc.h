#ifndef _BOOT_REGION_ALLOC_H
#define _BOOT_REGION_ALLOC_H

#include <nyx/types.h>

struct memregion {
    u64 base;
    u64 size;
};

/* this alloctor is meant to be used to get large buffers that
 * may layer be used to initialize some finer grained allocators.
 * overusing allocations from here can cause failed allocatios
 * later on, due to the low amount of space for the actual
 * region metadata.
 */
int              ra_init(u64 bi);
int              ra_add_region(struct memregion region);
int              ra_subtract_region(struct memregion region);
struct memregion ra_get_region(u64 size, u64 align);

void ra_dump_regions();

#endif
