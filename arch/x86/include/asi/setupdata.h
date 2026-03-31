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
#define MMAP_TYPE_BOOT_RECLAIMABLE 6

struct mmap_entry {
    u64 addr;
    u64 size;
    u32 type;
} __packed;

static const char *__mmap_type_strtab[] = {
        [MMAP_TYPE_NONE]             = "none",
        [MMAP_TYPE_AVAILABLE]        = "available",
        [MMAP_TYPE_RESERVED]         = "reserved",
        [MMAP_TYPE_ACPI_RECLAIMABLE] = "acpi reclaimable",
        [MMAP_TYPE_NVS]              = "nvs",
        [MMAP_TYPE_UNUSABLE]         = "unusable",
        [MMAP_TYPE_BOOT_RECLAIMABLE] = "boot reclaimable",
};

static inline const char *mmap_type_string(struct mmap_entry *entry) {
    return __mmap_type_strtab[entry->type];
}

static inline int mmap_is_memory(struct mmap_entry *entry) {
    return entry->type == MMAP_TYPE_AVAILABLE;
}

#endif
