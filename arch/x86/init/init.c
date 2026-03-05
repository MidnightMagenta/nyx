#include <asi/boot.h>
#include <nyx/compiler.h>
#include <nyx/early_printk.h>
#include <nyx/linkage.h>
#include <nyx/types.h>
#include <nyx/util.h>

void __init kernel_init(u64 bi) {
    u32             bi_total_size = *(u32 *) bi;
    struct mb2_tag *bi_tag        = (struct mb2_tag *) (bi + 8);
    struct mb2_tag *bi_end        = (struct mb2_tag *) ((char *) bi_tag + bi_total_size);

    while (bi_tag->type != MB2_TAG_END && bi_tag <= bi_end) {
        if (bi_tag->type != MB2_TAG_MMAP) { goto next_tag; } // ignore non mmap tags

        struct mb2_tag_mmap *mmap_tag = (struct mb2_tag_mmap *) bi_tag;

        int e_num = 0;
        for (u8 *p = (u8 *) mmap_tag->entries; p < (u8 *) mmap_tag + mmap_tag->size; p += mmap_tag->entry_size) {
            struct mb2_mmap_entry *mmap_entry = (struct mb2_mmap_entry *) p;

            u64 region_start = mmap_entry->addr;
            u64 region_end   = mmap_entry->addr + mmap_entry->len;

            early_printk("Entry %d\n  mem type: %d\n  start: 0x%lx\n  end: 0x%lx\n",
                         e_num,
                         mmap_entry->type,
                         region_start,
                         region_end);
            e_num++;
        }

    next_tag:
        bi_tag = (struct mb2_tag *) ((char *) bi_tag + ALIGN_UP(bi_tag->size, 8));
    }
}
