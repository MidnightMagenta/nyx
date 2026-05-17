#ifndef _NYX_SCHED_H
#define _NYX_SCHED_H

#include <nyx/list.h>
#include <nyx/types.h>
#include <uapi/posix_types.h>

#include <asi/proc.h>

enum task_state {
    SLEEPING,
    RUNNABLE,
    RUNNING,
    ZOMBIE,
};

struct task_struct {
    struct list_head list;

    enum task_state     state;
    struct proc_context context;
    pid_t               pid;
    void               *stack;

    struct list_head task_list;
};

struct task_struct *get_current_task();
void                schedule();

#define yield() schedule()

struct task_struct *task_create(void (*entry)(void *), virt_addr_t context);
void                task_exit();

int need_resched();

#endif
