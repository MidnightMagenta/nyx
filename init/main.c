#include <nyx/printk.h>

#ifdef CONFIG_KERNEL_TESTS
extern void __do_kernel_tests();
#else
#define __do_kernel_tests() ;
#endif

extern void init_memory();

void start_kernel() {
    init_memory();

    __do_kernel_tests();

    pr_dbg("finish\n");
    while (1) { __asm__ volatile("hlt"); }
}
