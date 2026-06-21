#include <nyx/kernel.h>
#include <nyx/kthread.h>
#include <nyx/stddef.h>
#include <nyx/wait.h>

#include <asi/bug.h>
#include <asi/irq.h>

#ifdef CONFIG_KERNEL_TESTS
extern void __do_kernel_tests();
#else
#define __do_kernel_tests()
#endif

static void __idle_task_fn() {
    for (;;) { asm volatile("hlt"); }
}

extern void setup_arch();
extern void init_memory();
extern void init_irq();
extern void init_timer();
extern void init_sched();

int testthrdslp;

void test_kthread_a(void *) {
    volatile int kt_a = 0;
    while (1) {
        if (kt_a == 10000000) {
            wakeup_one(&testthrdslp);
            kt_a = 0;
        }
        kt_a++;
    }
}

void test_kthread_b(void *) {
    while (1) {
        sleep_setup(&testthrdslp, "test");
        sleep_finish(1);
        printk("b woke up\n");
    }
}

void setup_test_kthreads() {
    kthread_create(test_kthread_a, NULL, "test_a");
    kthread_create(test_kthread_b, NULL, "test_b");
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

    setup_test_kthreads();

    arch_irq_enable();

    __idle_task_fn();

    BUG();
}
