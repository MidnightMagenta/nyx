#include <nyx/errno.h>
#include <nyx/kthread.h>
#include <nyx/sched.h>
#include <nyx/types.h>

extern void kthread_trampoline(void *);

int kthread_create(void (*entry)(void)) {
    struct task_struct *kthread = task_create(kthread_trampoline, (virt_addr_t) entry);
    if (!kthread) { return -ENOSPC; }
    return 0;
}

void kthread_exit() {
    task_exit();
}
