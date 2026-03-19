#include <mm/memblock.h>

void start_kernel() {
    memblock_init();
    while (1) { __asm__ volatile("hlt"); }
}
