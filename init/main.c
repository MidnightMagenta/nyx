#include <mm/memblock.h>
#include <nyx/types.h>

void start_kernel(phys_addr_t bootparams) {
    memblock_init(bootparams);
    while (1) { __asm__ volatile("hlt"); }
}
