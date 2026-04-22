#ifndef _ASI_MMAP_H
#define _ASI_MMAP_H

#include <asi/setupdata.h>
#include <nyx/compiler.h>
#include <nyx/types.h>

#include <asi/page.h>

extern struct mmap_map mmap_map;

#define mmap_type_is_memory(t) ((t) == MMAP_TYPE_RAM)
#define mmap_is_memory(entry)  mmap_type_is_memory((entry)->type)

int   mmap_any_mapped(u64 start, u64 end, mmap_type_t type);
int   mmap_all_mapped(u64 start, u64 end, mmap_type_t type);
void  mmap_add_region(u64 start, u64 end, mmap_type_t type);
int   mmap_get_next(u64 *start, u64 *end, mmap_type_t *type, u64 *idx);
pfn_t mmap_get_highest_pfn();
pfn_t mmap_get_lowest_pfn();
void  mmap_setup_map();

#endif
