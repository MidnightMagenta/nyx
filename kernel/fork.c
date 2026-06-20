#include <mm/physmem.h>
#include <mm/vmspace.h>
#include <nyx/atomic.h>
#include <nyx/current.h>
#include <nyx/errno.h>
#include <nyx/list.h>
#include <nyx/proc.h>
#include <nyx/sched.h>
#include <nyx/string.h>
#include <uapi/posix_types.h>

#include <asi/address.h>

extern void arch_fork(struct thread *t1, struct thread *t2, void (*func)(void *), void *arg);

static struct process *new_process(struct process *parent) {
    struct process *pr = alloc_proc(M_SLEEPOK);

    if (!pr) { return NULL; }

    list_init(&pr->thrds_list);
    list_init(&pr->children_head);
    list_init(&pr->child_node);

    pr->flags = 0;
    pr->state = PS_NEW;
    pr->pid   = get_pid();
    atomic_store_explicit(&pr->live_thrd_cnt, 0, ATOMIC_RELAXED);

    pr->parent = parent;
    list_add_tail(&pr->child_node, &parent->children_head);

    pr->xstatus = 0;

    memcpy(pr->name, parent->name, PROC_NAME_LEN);

    pr->mm = vmspace_fork(parent);

    list_add_tail(&pr->gproc_node, &proc_list);

    return pr;
}

static struct thread *new_thread(struct process *parent) {
    struct thread *newt = alloc_thread(M_SLEEPOK);
    phys_addr_t    kstack_phys;

    if (!newt) { return NULL; }

    newt->flags = 0;
    newt->state = TS_NEW;

    kstack_phys = pm_get_zeroed_page(M_SLEEPOK);
    if (kstack_phys == INVALID_PHYS_ADDR) {
        free_thread(newt);
        return NULL;
    }
    newt->kstack = __va(kstack_phys);

    newt->tid   = get_tid();
    newt->proc  = parent;
    newt->wchan = NULL;
    list_init(&newt->qnode);
    list_init(&newt->thrd_node);
    list_init(&newt->gthrd_node);

    list_add_tail(&newt->thrd_node, &parent->thrds_list);
    list_add_tail(&newt->gthrd_node, &thread_list);

    return newt;
}

int fork1(struct thread  *curp,
          int             flags,
          void            (*func)(void *),
          void           *arg,
          register_t     *retval,
          struct thread **newproc) {
    struct process *curpr = curp->proc;
    struct process *newpr;
    struct thread  *newthrd;

    (void) flags;

    newpr = new_process(curpr);
    if (!newpr) { return -ENOSPC; }

    newthrd = new_thread(newpr);
    if (!newthrd) {
        free_proc(newpr);
        return -ENOSPC;
    }

    arch_fork(curp, newthrd, func, arg);

    if (newproc) { *newproc = newthrd; }
    if (retval) { *retval = newpr->pid; }

    newpr->state = PS_NORMAL;
    thread_make_runnable(newthrd);

    return 0;
}
