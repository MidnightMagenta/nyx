#ifndef _NYX_SCHED_H
#define _NYX_SCHED_H

#include <nyx/compiler.h>
#include <nyx/list.h>
#include <nyx/types.h>
#include <uapi/posix_types.h>

#include <asi/proc.h>

#define TASK_NAME_LEN 32

enum task_state {
    TASK_NEW,
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
    char             name[TASK_NAME_LEN];
};

struct task_struct *get_current_task();
void                schedule();

#define yield() schedule()

extern void         arch_init_task(struct task_struct *task, void *stack, void (*entry)(void *), virt_addr_t context);
struct task_struct *task_alloc(const char *name);
void                task_make_runnable(struct task_struct *task);
void __noreturn     task_exit();

int need_resched();

#endif
