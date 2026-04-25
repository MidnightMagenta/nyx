#include <mm/memblock.h>
#include <nyx/linkage.h>
#include <nyx/types.h>

#include <asi/mmap.h>
#include <asi/setupdata.h>

extern void idt_setup_interrupts();

static void __init add_memblock_regions() {
    u64         rb, re, idx;
    mmap_type_t rt;

    idx = 0;
    while (mmap_get_next(&rb, &re, &rt, &idx)) {
        memblock_add(rb, re - rb);
        if (!mmap_type_is_memory(rt)) { memblock_reserve(rb, re - rb); }
    }

    memblock_trim();
}

void __init setup_arch() {
    idt_setup_interrupts();
    mmap_setup_map();
    memblock_init();
    add_memblock_regions();
}
