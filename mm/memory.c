#include <mm/memblock.h>
#include <mm/mmzone.h>
#include <nyx/linkage.h>

struct pg_data_s  contigmem_pagedata;
struct pg_data_s *pgdata = &contigmem_pagedata;

extern void init_page_alloc();
extern void arch_init_memory();

void __init init_memory() {
    arch_init_memory();
    init_page_alloc();
    memblock_free_all();
}
