#include <asi/system.h>
#include <nyx/kernel.h>

#include <mm/physmem.h>
#include <mm/virtmem.h>

#include <nyx/tick.h>

#ifdef CONFIG_KERNEL_TESTS
extern void __do_kernel_tests();
#else
#define __do_kernel_tests()
#endif

extern void setup_arch();
extern void init_memory();
extern void init_irq();
extern void init_timer();

void start_kernel() {
    pr_info("kernel build ID: %s\n", NYX_BUILD_ID);
    setup_arch();
    init_memory();
    init_irq();
    init_timer();

    __do_kernel_tests();

    pr_dbg("finish\n");

    sti();
    while (1) { printk("uptime: %d ms\n", uptime_ms()); }

    hcf();
}
