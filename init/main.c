#include <mm/memblock.h>
#include <mm/pmm.h>
#include <nyx/printk.h>

#ifdef CONFIG_KERNEL_TESTS
extern void __do_kernel_tests();
#else
#define __do_kernel_tests() ;
#endif

extern void pm_init();

void start_kernel() {
    memblock_init();
    pm_init();

    __do_kernel_tests();

    pr_dbg("finish\n");
    while (1) { __asm__ volatile("hlt"); }
}
