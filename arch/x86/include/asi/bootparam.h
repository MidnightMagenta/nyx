#ifndef _ASI_BOOT_H
#define _ASI_BOOT_H

#include <asi/page.h>
#include <asi/setupdata.h>
#include <nyx/compiler.h>
#include <nyx/types.h>
#include <nyx/util.h>

#define BP_VERSION_NONE 0
#define BP_VERSION_1    1

#define MMAP_MAX_ENTRIES 128

struct boot_params {
    u32               version;                /* 0x000 */
    u32               size;                   /* 0x004 */
    struct mmap_entry mmap[MMAP_MAX_ENTRIES]; /* 0x008 */
} __packed;

#endif
