#ifndef _ASI_MMAP_H
#define _ASI_MMAP_H

#include <nyx/compiler.h>
#include <nyx/types.h>

#define MMAP_MAX 128

#define MMAP_TYPE_NONE             0
#define MMAP_TYPE_RAM              1
#define MMAP_TYPE_RESERVED         2
#define MMAP_TYPE_ACPI_RECLAIMABLE 3
#define MMAP_TYPE_NVS              4
#define MMAP_TYPE_UNUSABLE         5
#define MMAP_TYPE_BOOT_RECLAIMABLE 6

typedef u32 mmap_type_t;

struct mmap_entry {
    u64         addr;
    u64         size;
    mmap_type_t type;
} __packed;

const char *mmap_type_string(mmap_type_t type);

#define mmap_is_memory(entry) ((entry)->type == MMAP_TYPE_RAM)

#endif
