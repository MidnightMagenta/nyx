#include <mm/physmem.h>
#include <mm/slab.h>
#include <nyx/bitmap.h>
#include <nyx/compiler.h>
#include <nyx/current.h>
#include <nyx/kernel.h>
#include <nyx/linkage.h>
#include <nyx/list.h>
#include <nyx/proc.h>
#include <nyx/sched.h>
#include <nyx/string.h>
#include <nyx/types.h>
#include <uapi/posix_types.h>

#include <asi/address.h>
#include <asi/bug.h>
#include <asi/irq.h>

extern struct thread proc0;
extern void          context_switch(struct thread *prev, struct thread *next);

extern void proc_init();
extern void proc0_init();
void __init map_kernel();

void init_sched() {
    proc0_init();
    map_kernel();
    get_pcpu()->current_task         = &proc0;
    get_pcpu()->scheds.cpu_idle_proc = &proc0;
    get_pcpu()->scheds.flags         = 0;
    list_init(&get_pcpu()->scheds.runq);
    proc_init();
}

static struct thread *pick_next(struct sched_percpu *schedc) {
    struct thread *next = NULL;

    if (!list_is_empty(&schedc->runq)) {
        next = list_first_entry(&schedc->runq, struct thread, qnode);
        if (next->qnode.next != &schedc->runq) { list_del(&next->qnode); }
    }

    return next ? next : schedc->cpu_idle_proc;
}

void schedule() {
    struct sched_percpu *schedc = &get_pcpu()->scheds;
    struct thread       *prev, *next;
    flags_t              flags;

    if (list_is_empty(&schedc->runq) && current()->state == TS_RUNNING) { return; }

    flags = arch_irq_save();

    prev = current();
    next = pick_next(schedc);

    schedc->flags &= ~SCHED_NEED_RESCHED;

    if (prev == next) {
        arch_irq_restore(flags);
        return;
    }

    if (prev->state == TS_RUNNING) {
        prev->state = TS_RUNNABLE;
        if (prev != schedc->cpu_idle_proc) { list_add_tail(&prev->qnode, &schedc->runq); }
    }

    get_pcpu()->current_task = next;
    next->state              = TS_RUNNING;

    context_switch(prev, next);

    arch_irq_restore(flags);
}

void thread_make_runnable(struct thread *t) {
    struct sched_percpu *schedc = &get_pcpu()->scheds;
    flags_t              flags;

    BUG_ON(!list_is_empty(&t->qnode));

    flags = arch_irq_save();

    t->state = TS_RUNNABLE;
    list_add_tail(&t->qnode, &schedc->runq);

    arch_irq_restore(flags);
}

void scheduler_tick() {
    if (!list_is_empty(&get_pcpu()->scheds.runq)) { get_pcpu()->scheds.flags |= SCHED_NEED_RESCHED; }
}
