#include <asi/system.h>
#include <nyx/kernel.h>
#include <nyx/kthread.h>
#include <nyx/stddef.h>

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

int kt_a;
int kt_b;
int kt_c;

void test_kthread_a(void *) {
    kt_a = 0;
    while (1) {
        if (kt_a == 500000) {
            printk("a\n");
            kt_a = 0;
        }
        kt_a++;
    }
}

void test_kthread_b(void *) {
    int kt_b = 0;
    while (1) {
        if (kt_b == 500000) {
            printk("b\n");
            kt_b = 0;
        }
        kt_b++;
    }
}

void test_kthread_c(void *) {
    int kt_c = 0;
    while (1) {
        if (kt_c == 500000) {
            printk("c\n");
            kt_c = 0;
        }
        kt_c++;
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

    kthread_create(test_kthread_a, NULL, "test_a");
    kthread_create(test_kthread_b, NULL, "test_b");
    kthread_create(test_kthread_c, NULL, "test_c");

    sti();


    __idle_task_fn();

    hcf();
}
