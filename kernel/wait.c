#include <nyx/current.h>
#include <nyx/linkage.h>
#include <nyx/list.h>
#include <nyx/proc.h>
#include <nyx/sched.h>

#include <asi/irq.h>
#include <asi/wchan.h>

struct list_head waittab[NR_SLEEP_BUCKETS];

void __init wait_init() {
    for (size_t i = 0; i < NR_SLEEP_BUCKETS; i++) { list_init(&waittab[i]); }
}

void unsleep(struct thread *t) {
    if (t->wchan != NULL) {
        list_del(&t->qnode);
        t->wchan = NULL;
        t->wmesg = NULL;
        t->state = TS_RUNNABLE;
        list_add_tail(&t->qnode, &get_pcpu()->scheds.runq);
    }
}

void sleep_setup(const volatile void *ident, const char *wmesg) {
    struct thread *t     = current();
    flags_t        flags = arch_irq_save(); // TODO: SMP - lock the scheduler

    t->wchan = ident;
    t->wmesg = wmesg;

    list_add_tail(&t->qnode, &waittab[WAITID(ident)]);

    t->state = TS_SLEEPING;

    arch_irq_restore(flags);
}

void sleep_finish(int do_sleep) {
    struct thread *t = current();

    if (t->wchan == NULL) { return; }
    if (do_sleep == 0) {
        unsleep(t);
        return;
    }

    schedule();
}

void wakeup(const volatile void *ident) {
    struct list_head *list = &waittab[WAITID(ident)];
    struct list_head *pos, *n;
    struct thread    *t;

    list_for_each_safe(pos, n, list) {
        t = list_first_entry(pos, struct thread, qnode);
        if (t->wchan == ident) { unsleep(t); }
    }
}

void wakeup_one(const volatile void *ident) {
    struct list_head *list = &waittab[WAITID(ident)];
    struct list_head *pos;
    struct thread    *t;

    list_for_each(pos, list) {
        t = list_entry(pos, struct thread, qnode);
        if (t->wchan == ident) {
            unsleep(t);
            return;
        }
    }
}
