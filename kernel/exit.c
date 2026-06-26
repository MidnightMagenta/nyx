#include <mm/physmem.h>
#include <mm/vmspace.h>
#include <nyx/current.h>
#include <nyx/list.h>
#include <nyx/panic.h>
#include <nyx/percpu.h>
#include <nyx/printk.h>
#include <nyx/proc.h>
#include <nyx/sched.h>
#include <nyx/syscall.h>
#include <nyx/wait.h>

#include <asi/bug.h>

#define pr_fmt(fmt) "syscall: " fmt

#ifdef CONFIG_DEBUG_SYSCALL
#define pr_exit_debug(fmt, ...) printk("syscall/exit:%d: " fmt, __LINE__, ##__VA_ARGS__)
#else
#define pr_exit_debug(fmt, ...) /* void */
#endif

static LIST_HEAD(deadqueue);

void proc_zap(struct process *pr);

int __noreturn sys_exit(struct thread *t, struct syscall_args *args, register_t *retval) {
    (void) retval;
    (void) t;
    do_exit(args->arg1, EXIT_NORMAL);
    __builtin_unreachable();
}

static void reparent_children(struct process *pr) {
    struct list_head *cur, *n;
    struct process   *child;

    list_for_each_safe(cur, n, &pr->children_head) {
        child = list_entry(cur, struct process, child_node);

        list_move_tail(&child->child_node, &initproc->proc->children_head);
        child->parent = initproc->proc;
    }
}

// TODO: major - assumes single threaded
void __noreturn do_exit(int code, int flags) {
    struct thread  *t = current();
    struct process *pr;
    (void) flags;

    pr = t->proc;
    if (t->proc->pid == 1) { panic("init exiting with %d", code); }

    atomic_fetch_and(&t->flags, TF_EXITING, ATOMIC_ACQ_REL);

    pr_exit_debug("process (pid: %d, name: %s) exiting with %d\n", pr->pid, pr->name, code);

    BUG_ON(refcount_get(&pr->live_thrd_cnt) != 1); // unimplemented
    atomic_fetch_and(&pr->flags, PF_EXITING, ATOMIC_ACQ_REL);

    pr->state = PS_ZOMBIE;
    t->state  = TS_ZOMBIE;

    pr->xstatus = code;

    reparent_children(pr);
    schedule();
}

void exit_tail(struct thread *t) {
    BUG_ON(!list_is_empty(&t->qnode));
    list_add_tail(&t->qnode, &deadqueue);
    wakeup(&deadqueue);
}

void reaper(void *arg) {
    struct thread  *t;
    struct process *pr;
    (void) arg;

    for (;;) {
        while (list_is_empty(&deadqueue)) {
            sleep_setup(&deadqueue, "reaper");
            sleep_finish(list_is_empty(&deadqueue));
        }

        t  = list_first_entry(&deadqueue, struct thread, qnode);
        pr = t->proc;

        pm_free_page((phys_addr_t) t->kstack);
        vmspace_put(pr->mm);

        free_thread(t);

        if (atomic_load_explicit(&pr->flags, ATOMIC_ACQUIRE) & PF_NOZOMBIE) {
            proc_zap(pr);
        } else {
            atomic_fetch_and(&pr->flags, PF_REALZOMBIE, ATOMIC_ACQ_REL);
            wakeup(pr->parent);
        }
    }
}

void proc_zap(struct process *pr) {
    free_proc(pr);
}
