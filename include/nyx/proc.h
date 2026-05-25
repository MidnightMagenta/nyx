#ifndef _NYX_PROC_H
#define _NYX_PROC_H

#include <mm/mm_types.h>
#include <uapi/posix_types.h>

#include <asi/proc.h>

#define TASK_NAME_LEN 32

enum task_state {
    ZOMBIE   = -2,
    DEAD     = -1,
    TASK_NEW = 0,
    SLEEPING,
    RUNNABLE,
    RUNNING,
};

struct task_struct {
    struct list_head list;

    enum task_state     state; // task is alive is state > 0
    struct proc_context context;
    struct vas_struct  *vas;
    struct vas_struct  *active_vas;
    pid_t               pid;
    void               *stack;

    struct list_head task_list;
    char             name[TASK_NAME_LEN];
};

#endif
