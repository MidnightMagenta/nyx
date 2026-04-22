#ifndef _ASI_SETUPDATA_H
#define _ASI_SETUPDATA_H

#include <nyx/compiler.h>
#include <nyx/types.h>

/* firmware memory maps usually have < 32 entries. 128 max entries
 * chosen conservatively */
#define MMAP_MAX_ENTRIES 128

/* Ordering of values within this enum wull have impact on the order
 * of type precedense in mmap_sanitise() when resolving overlaps */
enum mmap_type {
    MMAP_TYPE_RAM = 0,
    MMAP_TYPE_BOOT_RECLAIM,
    MMAP_TYPE_ACPI_RECLAIM,
    MMAP_TYPE_ACPI_NVS,
    MMAP_TYPE_RESERVED,
    MMAP_TYPE_BAD,
    MMAP_NR_TYPES,
};

/* mmap_type_t set to u32 to guarantee fixed width across compilers.
 * Valid values defined by enum mmap_type */
typedef u32 mmap_type_t;

struct mmap_entry {
    u64         addr;
    u64         size;
    mmap_type_t type;
} __packed;

struct mmap_map {
    u64               nr_entries;
    struct mmap_entry map[MMAP_MAX_ENTRIES];
};

#endif
