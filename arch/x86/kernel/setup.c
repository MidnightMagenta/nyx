#include <asi/mmap.h>
#include <mm/memblock.h>
#include <nyx/linkage.h>

extern void arch_init_memory();

static void __init add_memblock_regions() {
    u64         rb, re, idx;
    mmap_type_t rt;

    idx = 0;
    while (mmap_get_next(&rb, &re, &rt, &idx)) {
        memblock_add(rb, re - rb);
        if (!mmap_type_is_memory(rt)) { memblock_reserve(rb, re - rb); }
    }

    memblock_trim();
    memblock_print_regions();
}

void __init setup_arch() {
    mmap_setup_map();
    memblock_init();
    add_memblock_regions();
    arch_init_memory();
}
