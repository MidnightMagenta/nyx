#ifndef _NYX_SCHED_H
#define _NYX_SCHED_H

#include <sys/compiler.h>
#include <sys/list.h>
#include <sys/percpu.h>
#include <sys/proc.h>
#include <sys/sched_percpu.h>
#include <sys/types.h>

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
