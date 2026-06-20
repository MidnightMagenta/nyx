#ifndef _NYX_SCHED_H
#define _NYX_SCHED_H

#include <nyx/compiler.h>
#include <nyx/list.h>
#include <nyx/percpu.h>
#include <nyx/proc.h>
#include <nyx/sched_percpu.h>
#include <nyx/types.h>

#include <asi/cpu.h>

void             schedule();
struct cpu_info *sched_pickcpu(struct thread *t);
void             setrunqueue(struct cpu_info *cpu, struct thread *t);
void             rmrunqueue(struct thread *t);

#define yield() schedule()

static inline int need_resched() {
    return get_pcpu()->scheds.flags & SCHED_NEED_RESCHED;
}

#endif
