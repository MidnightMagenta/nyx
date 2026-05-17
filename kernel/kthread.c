#include <nyx/errno.h>
#include <nyx/kthread.h>
#include <nyx/sched.h>
#include <nyx/types.h>

extern void kthread_trampoline(void *);

int kthread_create(void (*entry)(void), const char *name) {
    struct task_struct *kthread = task_alloc(name);
    if (!kthread) { return -ENOSPC; }

    arch_init_task(kthread, kthread->stack, kthread_trampoline, (virt_addr_t) entry);
    task_make_runnable(kthread);

    return 0;
}

void kthread_exit() {
    task_exit();
}
