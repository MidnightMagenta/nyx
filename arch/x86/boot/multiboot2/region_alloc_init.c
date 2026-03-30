#include "../boot_utils.h"
#include "../region_alloc.h"
#include "multiboot2.h"
#include <asi/page.h>
#include <nyx/align.h>
#include <nyx/stddef.h>
#include <nyx/types.h>

static struct mb2_tag_mmap *get_mmap(u64 bi) {
    unsigned int    bi_size = *(unsigned int *) bi;
    struct mb2_tag *bi_tag  = (struct mb2_tag *) (bi + 8);
    struct mb2_tag *bi_end  = (struct mb2_tag *) (bi + bi_size);

    while (bi_tag <= bi_end && bi_tag->type != MB2_TAG_END) {
        if (bi_tag->type == MB2_TAG_MMAP) { return (struct mb2_tag_mmap *) bi_tag; }
        bi_tag = (struct mb2_tag *) ((char *) bi_tag + ALIGN_UP(bi_tag->size, 8));
    }

    return NULL;
}

extern char __image_start;
extern char __image_end;

int ra_init_imp(u64 bi, struct memregion *free_regions, size_t *region_count, const size_t region_array_size) {
    struct mb2_tag_mmap *mmap    = get_mmap(bi);
    unsigned int         bi_size = *(unsigned int *) bi;
    struct mb2_tag      *bi_tag  = (struct mb2_tag *) (bi + 8);
    struct mb2_tag      *bi_end  = (struct mb2_tag *) (bi + bi_size);

    memsetb(free_regions, 0, region_array_size);
    *region_count = 1;

    for (u8 *p = (u8 *) mmap->entries; p < (u8 *) mmap + mmap->size; p += mmap->entry_size) {
        struct mb2_mmap_entry *mmap_entry = (struct mb2_mmap_entry *) p;
        if (mmap_entry->type != MB2_MMAP_AVAILABLE) { continue; }
        if (ra_add_region((struct memregion) {.base = mmap_entry->addr, .size = mmap_entry->len}) != 0) { return 1; }
    }

    // reserve memory occupied by the kernel
    if (ra_subtract_region(
                (struct memregion) {.base = ALIGN_DOWN((u64) &__image_start, PAGE_SIZE),
                                    .size = ALIGN_UP(((u64) &__image_end - (u64) &__image_start), PAGE_SIZE)}) != 0) {
        return 1;
    }

    // reserve memory occupied by boot info
    if (ra_subtract_region((struct memregion) {.base = bi, .size = *(u32 *) bi}) != 0) { return 1; }

    // reserve other memory multiboot2 bootloaders don't mark
    while (bi_tag <= bi_end && bi_tag->type != MB2_TAG_END) {
        if (bi_tag->type == MB2_TAG_MODULE) {
            struct mb2_tag_module *bi_module = (struct mb2_tag_module *) bi_tag;
            if (ra_subtract_region(
                        (struct memregion) {.base = (u64) bi_module->mod_start,
                                            .size = ((u64) bi_module->mod_end - (u64) bi_module->mod_start)}) != 0) {
                return 1;
            }
        }
        bi_tag = (struct mb2_tag *) ((char *) bi_tag + ALIGN_UP(bi_tag->size, 8));
    }

    return 0;
}
