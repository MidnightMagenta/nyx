#ifndef _MM_MEMORY_H
#define _MM_MEMORY_H

#include <asi/page.h>

extern struct pg_data_s *pgdata;

#define pfn_to_page(pfn) (&pgdata->mem_map[pfn - pgdata->start_pfn])
#define page_to_pfn(pg)  ((pg - pgdata->mem_map) + pgdata->start_pfn)

#define phys_to_page(phys) (pfn_to_page(addr_to_pfn(phys)))
#define page_to_phys(pg)   (pfn_to_addr(page_to_pfn(pg)))

#endif
