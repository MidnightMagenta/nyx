#ifndef _ASI_BOOT_H
#define _ASI_BOOT_H

#include <asi/page.h>
#include <asi/setupdata.h>
#include <nyx/types.h>
#include <nyx/util.h>

#define MMAP_MAX_ENTRIES 128

struct boot_params {
    struct mmap_entry mmap[MMAP_MAX_ENTRIES]; /* 0x000 */
} __attribute__((packed));

#define BOOT_PARAM_SIZE ALIGN_UP(sizeof(boot_params), PAGE_SIZE)

#endif
