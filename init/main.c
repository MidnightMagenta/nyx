#include <asi/system.h>
#include <nyx/kernel.h>
#include <nyx/kthread.h>

#include <mm/physmem.h>
#include <mm/virtmem.h>

#ifdef CONFIG_KERNEL_TESTS
extern void __do_kernel_tests();
#else
#define __do_kernel_tests()
#endif

static void idle_task() {
    for (;;) { asm volatile("hlt"); }
}

extern void setup_arch();
extern void init_memory();
extern void init_irq();
extern void init_timer();
extern void init_sched();

void test_kthread_a() {
    int i = 0;
    while (1) {
        if (i == 500000) {
            printk("a\n");
            i = 0;
        }
        i++;
    }
}

void test_kthread_b() {
    int i = 0;
    while (1) {
        if (i == 500000) {
            printk("b\n");
            i = 0;
        }
        i++;
    }
}

void start_kernel() {
    pr_info("kernel build ID: %s\n", NYX_BUILD_ID);
    setup_arch();
    init_memory();
    init_irq();
    init_timer();
    init_sched();

    __do_kernel_tests();

    pr_dbg("finish\n");

    kthread_create(test_kthread_a);
    kthread_create(test_kthread_b);

    sti();


    idle_task();

    hcf();
}
