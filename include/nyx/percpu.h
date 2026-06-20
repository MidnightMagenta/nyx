#ifndef _NYX_PERCPU_H
#define _NYX_PERCPU_H

#include <nyx/sched_percpu.h>
#include <nyx/types.h>

#include <asi/cpu.h>

struct percpu {
    struct percpu      *self;
    register_t          rsp0;
    register_t          ursp;
    struct thread      *current_task;
    struct sched_percpu scheds;
    int                 preempt_count;
};

extern struct percpu __pcpu[MAX_CPUS];

struct percpu *get_pcpu();

#endif
