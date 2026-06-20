#ifndef _NYX_PROC_H
#define _NYX_PROC_H

#include <mm/mm_types.h>
#include <nyx/atomic.h>
#include <nyx/wait.h>
#include <uapi/posix_types.h>

#include <asi/proc.h>

#define PROC_NAME_LEN 32

struct process;
struct thread;

struct thread {
    unsigned long flags;

    struct list_head qnode;

    enum thread_state {
        TS_NEW,
        TS_RUNNABLE,
        TS_RUNNING,
        TS_SLEEPING,
        TS_ZOMBIE,
    } state;

    struct thread_context ctx;
    void                 *kstack;
    pid_t                 tid;
    struct process       *proc;
    void                 *wchan;

    struct list_head thrd_node;
    struct list_head gthrd_node;
};

struct process {
    unsigned long flags;

    enum proc_state {
        PS_NEW,
        PS_NORMAL,
        PS_ZOMBIE,
    } state;

    struct vmspace  *mm;
    pid_t            pid;
    atomic_t         live_thrd_cnt;
    struct list_head thrds_list;
    struct process  *parent;
    struct list_head children_head;
    struct list_head child_node;
    int              xstatus;

    struct list_head gproc_node;
    char             name[PROC_NAME_LEN];
};

extern struct list_head proc_list;
extern struct list_head thread_list;

static inline void thread_set_state(struct thread *t, enum thread_state state) {
    t->state = state;
}

static inline void proc_set_state(struct process *p, enum proc_state state) {
    p->state = state;
}

struct process *alloc_proc(int gfp_flags);
struct thread  *alloc_thread(int gfp_flags);
void            free_proc(struct process *proc);
void            free_thread(struct thread *thrd);

pid_t get_pid();
void  put_pid(pid_t pid);
pid_t get_tid();
void  put_tid(pid_t tid);

int fork1(struct thread *curp, int flags, void (*func)(void *), void *arg, register_t *retval, struct thread **newproc);

#endif
