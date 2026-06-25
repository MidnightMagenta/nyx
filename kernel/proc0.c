#include <mm/mm_types.h>
#include <nyx/atomic.h>
#include <nyx/list.h>
#include <nyx/percpu.h>
#include <nyx/proc.h>
#include <nyx/refcount.h>

extern char init_stack_top[];

struct vmspace __proc0_vmspace;
struct thread  proc0;
struct process proc0_proc;

void proc0_init() {
    __proc0_vmspace.pgd = NULL;
    refcount_init(&__proc0_vmspace.refcount, 1);
    list_init(&__proc0_vmspace.vma_regions);

    atomic_store_explicit(&proc0.flags, 0, ATOMIC_RELAXED);
    proc0.state  = TS_RUNNING;
    proc0.kstack = init_stack_top;
    proc0.tid    = 0;
    proc0.proc   = &proc0_proc;
    proc0.wchan  = NULL;

    list_init(&proc0.qnode);
    list_init(&proc0.thrd_node);
    list_init(&proc0.gthrd_node);
    list_add_tail(&proc0.gthrd_node, &thread_list);

    atomic_store_explicit(&proc0_proc.flags, 0, ATOMIC_RELAXED);
    proc0_proc.state = PS_NORMAL;
    proc0_proc.mm    = &__proc0_vmspace;
    proc0_proc.pid   = 0;
    refcount_init(&proc0_proc.live_thrd_cnt, 1);
    proc0_proc.parent  = NULL;
    proc0_proc.xstatus = 0;

    list_init(&proc0_proc.thrds_list);
    list_init(&proc0_proc.children_head);
    list_init(&proc0_proc.child_node);
    list_init(&proc0_proc.gproc_node);
    list_add_tail(&proc0.thrd_node, &proc0_proc.thrds_list);
    list_add_tail(&proc0_proc.gproc_node, &proc_list);
}
