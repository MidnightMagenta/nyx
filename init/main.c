#include <mm/memblock.h>
#include <mm/pmm.h>
#include <nyx/printk.h>

extern void pm_init();

void start_kernel() {
    memblock_init();
    pm_init();

    pr_dbg("free pages before allocation: %ld\n", pm_getstat(PM_STAT_FREE));

    phys_addr_t alloc = __pm_get_pages(0, 0);

    pr_dbg("allocated 0x%lx\nfree pages after allocation: %ld\n", alloc, pm_getstat(PM_STAT_FREE));

    pm_free_pages(alloc);

    pr_dbg("free pages after free: %ld\n", pm_getstat(PM_STAT_FREE));

    while (1) { __asm__ volatile("hlt"); }
}
