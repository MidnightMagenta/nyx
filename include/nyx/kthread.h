#ifndef _NYX_KTHREAD_H
#define _NYX_KTHREAD_H

#include <nyx/errno.h>
#include <nyx/sched.h>

static inline int kthread_create(void (*entry)(void)) {
    struct task_struct *kthread = task_create(entry);
    if (!kthread) { return -ENOSPC; }
    return 0;
}

static inline void kthread_exit() {
    task_exit();
}

#endif
