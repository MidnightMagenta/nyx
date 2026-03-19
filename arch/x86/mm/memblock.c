#include <asi/bootparam.h>
#include <asi/page.h>
#include <asi/setupdata.h>
#include <mm/memblock.h>
#include <nyx/panic.h>
#include <nyx/types.h>
#include <nyx/util.h>

extern char __image_start;
extern char __image_end;

void memblock_init() {
    int                 res;
    struct boot_params *bp = (struct boot_params *) bootparams;

    const struct mmap_entry *entry = bp->mmap;
    const struct mmap_entry *end =
            (struct mmap_entry *) ((char *) bp->mmap + bp->mmap_entry_count * sizeof(struct mmap_entry));

    while (entry < end && entry->type != MMAP_TYPE_NONE) {
        if (entry->type == MMAP_TYPE_AVAILABLE) {
            if ((res = memblock_add_memory(entry->addr, entry->size)) != 0) {
                early_panic("memblock: failed to add memory. ecode: %d", res);
            }
        }
        entry = (struct mmap_entry *) ((char *) entry + sizeof(struct mmap_entry));
    }

    if ((res = memblock_reserve((phys_addr_t) bootparams, ALIGN_UP(sizeof(struct boot_params), PAGE_SIZE))) != 0) {
        early_panic("memblock: failed to reserve boot_params. ecode: %d", res);
    }

    u64 kernel_phys_base = bp->kernel_load_base;
    u64 kernel_phys_end  = ((u64) &__image_end - (u64) &__image_start) + bp->kernel_load_base;

    if ((res = memblock_reserve(kernel_phys_base, kernel_phys_end - kernel_phys_base)) != 0) {
        early_panic("memblock: failed to reserve kernel. ecode: %d", res);
    }
}
