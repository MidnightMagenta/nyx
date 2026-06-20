#ifndef _NYX_SCHED_H
#define _NYX_SCHED_H

#include <nyx/compiler.h>
#include <nyx/list.h>
#include <nyx/percpu.h>
#include <nyx/proc.h>
#include <nyx/sched_percpu.h>
#include <nyx/types.h>

void schedule();
void thread_make_runnable(struct thread *t);

#define yield() schedule()

// extern void         arch_init_task(struct task_struct *task, void *stack,
//                     void (*entry)(void *), virt_addr_t context);
// struct task_struct *task_alloc(const char *name);
// void                task_free(struct task_struct *task);
// void __noreturn     task_exit();

static inline int need_resched() {
    return get_pcpu()->scheds.flags & SCHED_NEED_RESCHED;
}

#endif
