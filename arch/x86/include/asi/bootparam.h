#ifndef _ASI_BOOT_H
#define _ASI_BOOT_H

#include <asi/mmap.h>
#include <nyx/compiler.h>
#include <nyx/types.h>

#define BP_VERSION_NONE 0
#define BP_VERSION_1    1

struct boot_params {
    u32             version; /* 0x000 */
    u32             size;    /* 0x004 */
    struct mmap_map mem_map;
    u64             kernel_load_base; /* 0xA10 */
} __packed;

extern struct boot_params *bootparams;

#endif
