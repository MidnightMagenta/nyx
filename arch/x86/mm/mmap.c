#include <asi/mmap.h>
#include <nyx/linkage.h>

static const char *__mmap_type_strtab[] = {
        [MMAP_TYPE_NONE]             = "none",
        [MMAP_TYPE_RAM]              = "available",
        [MMAP_TYPE_RESERVED]         = "reserved",
        [MMAP_TYPE_ACPI_RECLAIMABLE] = "acpi reclaimable",
        [MMAP_TYPE_NVS]              = "nvs",
        [MMAP_TYPE_UNUSABLE]         = "unusable",
        [MMAP_TYPE_BOOT_RECLAIMABLE] = "boot reclaimable",
};

__init const char *mmap_type_string(mmap_type_t type) {
    return __mmap_type_strtab[type];
}

// TODO: memblock gets set up by this code. At the point memblock is ready, we throw away this map, and memblock's
// internal map will be used as the kernel's interim memory map
