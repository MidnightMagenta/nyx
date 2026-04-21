#include <asi/bootparam.h>
#include <asi/mmap.h>
#include <asi/page.h>
#include <mm/memblock.h>
#include <nyx/align.h>
#include <nyx/panic.h>
#include <nyx/types.h>

void init_memblock() {
    // int                 res;
    // struct boot_params *bp = (struct boot_params *) bootparams;
    //
    // const struct mmap_entry *entry = bp->mem_map.map;
    // const struct mmap_entry *end =
    //         (struct mmap_entry *) ((char *) bp->mem_map.map + bp->mem_map.nr_entries * sizeof(struct mmap_entry));
    //
    // while (entry < end && entry->type != MMAP_TYPE_NONE) {
    //     if (entry->type == MMAP_TYPE_RAM) {
    //         if ((res = memblock_add_memory(entry->addr, entry->size)) != 0) {
    //             early_panic("memblock: failed to add memory. ecode: %d", res);
    //         }
    //     }
    //     entry = (struct mmap_entry *) ((char *) entry + sizeof(struct mmap_entry));
    // }
    //
    // if ((res = memblock_reserve((phys_addr_t) bootparams, ALIGN_UP(sizeof(struct boot_params), PAGE_SIZE))) != 0) {
    //     early_panic("memblock: failed to reserve boot_params. ecode: %d", res);
    // }
}
