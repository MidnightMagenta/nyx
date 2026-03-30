#include <mm/mm_types.h>
#include <mm/mmzone.h>
#include <nyx/linkage.h>

struct pg_data_s  contigmem_pagedata;
struct pg_data_s *pgdata = &contigmem_pagedata;

extern void init_memblock();
extern void init_memmap();
extern void init_zones();
extern void init_page_alloc();

void __init init_memory() {
    init_memblock();
    init_memmap();
    init_zones();
    init_page_alloc();
}
