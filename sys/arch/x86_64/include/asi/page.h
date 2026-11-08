#ifndef _ASI_PAGE_H
#define _ASI_PAGE_H

#include <nyx/align.h>
#include <nyx/limits.h>
#include <nyx/types.h>

#define PAGE_SIZE  4096
#define PAGE_SHIFT 12

typedef u64 pfn_t;

#define PFN_MAX U64_MAX

#define PG_ALIGN_UP(a) ALIGN_UP((a), PAGE_SIZE)
#define PG_ALIGN_DN(a) ALIGN_DOWN((a), PAGE_SIZE)

#define addr_to_pfn(a) ((a) >> PAGE_SHIFT)
#define pfn_to_addr(n) ((n) << PAGE_SHIFT)

#endif
