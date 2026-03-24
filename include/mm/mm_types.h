#ifndef _MM_MM_TYPES_H
#define _MM_MM_TYPES_H

#include <nyx/types.h>

#define PAGE_FLAG_HEAD (1 << 0)
#define PAGE_FLAG_TAIL (1 << 1)

struct page {
    u64 flags;

    u16 order;
    u8  _pad[6];
};

#endif
