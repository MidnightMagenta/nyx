#include <mm/physmem.h>
#include <mm/slab.h>
#include <nyx/bitmap.h>
#include <nyx/kernel.h>
#include <nyx/linkage.h>
#include <nyx/sched.h>
#include <nyx/string.h>

#include <asi/address.h>

extern void idle_task();

static struct task_struct idle_task_struct = {
        .list = (struct list_head) LIST_HEAD_INIT(idle_task_struct.list),
};

struct {
    DECLARE_STATIC_BITMAP(bitmap, PID_MAX);
} pid_map;

kmem_cache_t *task_struct_cache;
static LIST_HEAD(runqueue);
static struct task_struct *current = &idle_task_struct;
int                        need_reschedule;

extern char init_stack_top[];

void __init init_sched() {
    memset(&pid_map, 0, ARRAY_SIZE(pid_map.bitmap));
    bm_set(pid_map.bitmap, 0);

    task_struct_cache = kmem_create_cache("task_struct",
                                          sizeof(struct task_struct),
                                          _Alignof(struct task_struct),
                                          NULL,
                                          NULL,
                                          GFP_ATOMIC);

    idle_task_struct.stack  = init_stack_top;
    idle_task_struct.state  = RUNNABLE;
    idle_task_struct.killed = 0;
    idle_task_struct.pid    = 0;
}

pid_t get_pid() {
    size_t pid = 1;
    while (bm_get(pid_map.bitmap, pid)) { pid++; }
    bm_set(pid_map.bitmap, pid);
    return pid;
}

extern void arch_init_task(struct task_struct *task, void *stack, void (*entry)(void));

static struct task_struct *alloc_task_struct(void (*entry)(void)) {
    struct task_struct *task = kmem_cache_alloc(task_struct_cache, 0);
    memset(task, 0, sizeof(struct task_struct));
    task->stack = __va(pm_get_zeroed_page(GFP_ATOMIC));
    task->pid   = get_pid();
    arch_init_task(task, task->stack, entry);
    return task;
}

struct task_struct *task_create(void (*entry)(void)) {
    struct task_struct *task = alloc_task_struct(entry);
    list_add_tail(&task->list, &runqueue);
    task->state = RUNNABLE;
    return task;
}

// TODO: need a reaper thread that cleans up zombie tasks
void task_exit() {
    struct task_struct *task = get_current_task();
    task->state              = ZOMBIE;
    task->killed             = 1;
    schedule();
}

static struct task_struct *pick_next() {
    struct task_struct *task;
    struct list_head   *cur;

    list_for_each(cur, &runqueue) {
        task = list_entry(cur, struct task_struct, list);
        if (task->state == RUNNABLE) {
            list_move_tail(&task->list, &runqueue);
            return task;
        }
    }

    return &idle_task_struct;
}

struct task_struct *get_current_task() {
    return current;
}

extern void switch_to(struct proc_context *prev, struct proc_context *next);

void schedule() {
    struct task_struct *cur  = get_current_task();
    struct task_struct *next = pick_next();

    need_reschedule = 0;

    if (cur->state == RUNNING) { cur->state = RUNNABLE; }
    next->state = RUNNING;

    current = next;

    switch_to(&cur->context, &next->context);
}

void scheduler_tick() {
    need_reschedule = 1;
}

int need_resched() {
    return need_reschedule;
}
