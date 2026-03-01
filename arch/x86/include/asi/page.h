#ifndef _ASI_PAGE_H
#define _ASI_PAGE_H

#define PAGE_SIZE  4096
#define PAGE_SHIFT 12

#define addr_to_pfn(a) ((a) >> PAGE_SHIFT)
#define pfn_to_addr(n) ((n) << PAGE_SHIFT)

#endif
