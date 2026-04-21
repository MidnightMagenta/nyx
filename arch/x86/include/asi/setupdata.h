#ifndef _ASI_SETUPDATA_H
#define _ASI_SETUPDATA_H

#include <nyx/compiler.h>
#include <nyx/types.h>

#define MMAP_MAX_ENTRIES 128

// #define MMAP_TYPE_NONE             0
// #define MMAP_TYPE_RAM              1
// #define MMAP_TYPE_RESERVED         2
// #define MMAP_TYPE_ACPI_RECLAIMABLE 3
// #define MMAP_TYPE_NVS              4
// #define MMAP_TYPE_UNUSABLE         5
// #define MMAP_TYPE_BOOT_RECLAIMABLE 6

enum mmap_type {
    MMAP_TYPE_RAM = 0,
    MMAP_TYPE_BOOT_RECLAIM,
    MMAP_TYPE_ACPI_RECLAIM,
    MMAP_TYPE_ACPI_NVS,
    MMAP_TYPE_RESERVED,
    MMAP_TYPE_BAD,
    MMAP_NR_TYPES,
};

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
