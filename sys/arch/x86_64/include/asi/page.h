#ifndef _ASI_PAGE_H
#define _ASI_PAGE_H

#include <sys/align.h>
#include <sys/limits.h>
#include <sys/types.h>

#define PAGE_SHIFT 12
#define PAGE_SIZE  (1ul << PAGE_SHIFT)
#define PAGE_MASK  (~(PAGE_SIZE - 1))

typedef u64 pfn_t;

#define PFN_MAX U64_MAX

#define PG_ALIGN_UP(a) ALIGN_UP((a), PAGE_SIZE)
#define PG_ALIGN_DN(a) ALIGN_DOWN((a), PAGE_SIZE)

#define addr_to_pfn(a)    ((unsigned long) (a) >> PAGE_SHIFT)
#define pfn_to_addr(n)    ((unsigned long) (n) << PAGE_SHIFT)
#define page_count(p)     ((unsigned long) (p) >> PAGE_SHIFT)
#define page_size(p)      ((unsigned long) (p) << PAGE_SHIFT)
#define offset_in_page(p) ((unsigned long) (p) & ~PAGE_MASK)

#endif
