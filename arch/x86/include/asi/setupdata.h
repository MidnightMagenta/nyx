#ifndef _ASI_SETUPDATA_H
#define _ASI_SETUPDATA_H

#include <nyx/compiler.h>
#include <nyx/types.h>

#define MMAP_TYPE_NONE             0
#define MMAP_TYPE_AVAILABLE        1
#define MMAP_TYPE_RESERVED         2
#define MMAP_TYPE_ACPI_RECLAIMABLE 3
#define MMAP_TYPE_NVS              4
#define MMAP_TYPE_UNUSABLE         5

struct mmap_entry {
    u64 addr;
    u64 size;
    u32 type;
} __packed;

#endif
