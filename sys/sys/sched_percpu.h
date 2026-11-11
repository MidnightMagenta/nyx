#ifndef _NYX_SHED_PERCPU_H
#define _NYX_SHED_PERCPU_H

#include <sys/list.h>
#include <sys/proc.h>
#include <sys/types.h>

#define SCHED_NEED_RESCHED (1 << 0)

struct sched_percpu {
    u64              flags;
    struct list_head runq;
    int              nr_run;
    struct thread   *cpu_idle_proc;
};

#endif
