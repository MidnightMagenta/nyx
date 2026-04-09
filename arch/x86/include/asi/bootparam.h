#ifndef _ASI_BOOT_H
#define _ASI_BOOT_H

#include <asi/mmap.h>
#include <nyx/compiler.h>
#include <nyx/types.h>

#define BP_VERSION_NONE 0
#define BP_VERSION_1    1

#define MMAP_MAX_ENTRIES MMAP_MAX

struct boot_params {
    u32               version;                /* 0x000 */
    u32               size;                   /* 0x004 */
    struct mmap_entry mmap[MMAP_MAX_ENTRIES]; /* 0x008 */
    u64               mmap_entry_count;       /* 0xA08 */
    u64               kernel_load_base;       /* 0xA10 */
} __packed;

extern struct boot_params *bootparams;

#endif
