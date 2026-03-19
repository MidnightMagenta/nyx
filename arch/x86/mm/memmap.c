#include <asi/bootparam.h>
#include <mm/memmap.h>
#include <nyx/linkage.h>
#include <nyx/panic.h>
#include <string.h>

static struct memmap memmap __initdata = {
        .region_cnt = (size_t) -1,
};

static void __init memmap_make() {
    if (memmap.region_cnt != (size_t) -1) { early_panic("memmap: can not initialize memmap twice"); }
    if (bootparams->mmap_entry_count >= MEMMAP_MAX_REGIONS) { early_panic("memmap: can not convert memory maps"); }

    memset(&memmap, 0, sizeof(struct memmap));

    memmap.region_cnt = bootparams->mmap_entry_count;
    for (size_t i = 0; i < bootparams->mmap_entry_count; ++i) {
        struct mem_region *r = &memmap.regions[i];
        r->base              = bootparams->mmap[i].addr;
        r->length            = bootparams->mmap[i].size;

        switch (bootparams->mmap[i].type) {
            case MMAP_TYPE_AVAILABLE:
                r->type = MEM_REGION_TYPE_AVAILABLE;
                break;
            case MMAP_TYPE_ACPI_RECLAIMABLE:
                [[fallthrough]];
            case MMAP_TYPE_NVS:
                [[fallthrough]];
            case MMAP_TYPE_RESERVED:
                r->type = MEM_REGION_TYPE_RESERVED;
                break;
            case MMAP_TYPE_UNUSABLE:
                r->type = MEM_REGION_TYPE_UNUSABLE;
                break;
            case MMAP_TYPE_BOOT_RECLAIMABLE:
                r->type = MEM_REGION_TYPE_BOOT_RECLAIMABLE;
                break;
            case MMAP_TYPE_NONE:
                early_panic("memmap: unexpected MMAP_TYPE_NONE entry");
            default:
                early_panic("memmap: unknown memory map type");
        }
    }
}

const struct memmap *memmap_get() {
    if (memmap.region_cnt == (size_t) -1) { memmap_make(); }
    return &memmap;
}
