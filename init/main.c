#include <nyx/kernel.h>
#include <nyx/kthread.h>
#include <nyx/panic.h>
#include <nyx/sched.h>
#include <nyx/stddef.h>
#include <nyx/wait.h>

#include <asi/bug.h>
#include <asi/irq.h>

#ifdef CONFIG_KERNEL_TESTS
extern void __do_kernel_tests();
#else
#define __do_kernel_tests()
#endif

static void __noreturn __idle_task_fn() {
    for (;;) { asm volatile("hlt"); }
}

extern void setup_arch();
extern void init_memory();
extern void init_irq();
extern void init_timer();
extern void init_sched();
extern void reaper(void *arg);
void        init_proc(void *arg);

extern struct thread proc0;
struct thread       *initproc;

int testthrdslp;

void test_kthread_a(void *) {
    volatile int kt_a = 0;
    while (1) {
        if (kt_a == 50000000) {
            wakeup(&testthrdslp);
            kt_a = 0;
        }
        kt_a++;
    }
}

void test_kthread_b(void *) {
    int i = 0;
    while (1) {
        sleep_setup(&testthrdslp, "test");
        sleep_finish(1);
        printk("b woke up\n");
        if (i++ == 5) { do_exit(0, EXIT_NORMAL); }
    }
}

void test_kthread_c(void *) {
    int i = 0;
    while (1) {
        sleep_setup(&testthrdslp, "test");
        sleep_finish(1);
        printk("c woke up\n");
        if (i++ == 5) { do_exit(0, EXIT_NORMAL); }
    }
}

void test_kthread_d(void *) {
    int i = 0;
    while (1) {
        sleep_setup(&testthrdslp, "test");
        sleep_finish(1);
        printk("d woke up\n");
        if (i++ == 5) { do_exit(0, EXIT_NORMAL); }
    }
}

void setup_test_kthreads() {
    kthread_create(test_kthread_a, NULL, "test_a");
    kthread_create(test_kthread_b, NULL, "test_b");
    kthread_create(test_kthread_c, NULL, "test_c");
    kthread_create(test_kthread_d, NULL, "test_d");
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

    if (do_fork(&proc0, FORK_NOZOMBIE | FORK_SHAREVM, &init_proc, NULL, NULL, &initproc) != 0) {
        panic("failed to start init");
    }

    kthread_create(reaper, NULL, "procreaper");

    setup_test_kthreads();

    arch_irq_enable();

    schedule(); // kick off the scheduler

    __idle_task_fn();

    BUG();
}

void init_proc(void *arg) {
    (void) arg;
    while (1) { hlt(); }
}
