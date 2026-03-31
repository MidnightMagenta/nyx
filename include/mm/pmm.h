#ifndef _MM_PMM_H
#define _MM_PMM_H

#include <asi/page.h>
#include <mm/mm_types.h>
#include <nyx/limits.h>
#include <nyx/stddef.h>
#include <nyx/string.h>
#include <nyx/types.h>

#define __GFP_DMA     (1 << 0)
#define __GFP_DMA32   (1 << 1)
#define __GFP_HIGHMEM (1 << 2)
#define __GFP_HIGH    (1 << 3)
#define __GFP_WAIT    (1 << 4)
#define __GFP_NOFAIL  (1 << 5)
#define __GFP_ZERO    (1 << 6)

#define GFP_DMA      (__GFP_DMA)
#define GFP_DMA32    (__GFP_DMA32)
#define GFP_ATOMIC   (__GFP_HIGH)
#define GPF_KERNEL   (__GFP_WAIT)
#define GFP_USER     (__GFP_WAIT)
#define GFP_HIGHUSER (__GFP_WAIT | __GFP_HIGHMEM)

struct page *pm_alloc_pages(u32 gfp_mask, u32 order);
#define pm_alloc_page(gfp_mask) pm_alloc_pages((gfp_mask), 0)

phys_addr_t __pm_get_free_pages(u32 gfp_mask, u32 order);
#define __pm_get_free_page(gfp_mask)        __pm_get_free_pages((gfp_mask), 0)
#define __pm_get_dma_pages(gfp_mask, order) __pm_get_free_pages((gfp_mask) | __GFP_DMA, (order))
#define pm_get_zeroed_page(gfp_mask)        __pm_get_free_pages((gfp_mask) | __GFP_ZERO, 0)

void __pm_free_pages(struct page *page, u32 order);
void pm_free_pages(phys_addr_t addr, u32 order);
#define __free_page(page) __pm_free_pages(page, 0)
#define free_page(addr)   pm_free_pages(addr, 0)

#endif
