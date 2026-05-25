#include <mm/physmem.h>
#include <mm/slab.h>
#include <nyx/bitmap.h>
#include <nyx/compiler.h>
#include <nyx/kernel.h>
#include <nyx/linkage.h>
#include <nyx/list.h>
#include <nyx/proc.h>
#include <nyx/sched.h>
#include <nyx/string.h>
#include <nyx/types.h>
#include <uapi/posix_types.h>

#include <asi/address.h>

extern struct task_struct idle_task;

struct {
    DECLARE_STATIC_BITMAP(bitmap, PID_MAX);
} pid_map;

kmem_cache_t *task_struct_cache;

static LIST_HEAD(task_list);
static LIST_HEAD(runqueue);


// HACK: global, SMP - these should be per-CPU
static struct task_struct *current = &idle_task;
volatile int               need_reschedule;

void __init init_sched() {
    memset(&pid_map, 0, ARRAY_SIZE(pid_map.bitmap));
    bm_set(pid_map.bitmap, 0);

    task_struct_cache = kmem_create_cache("task_struct",
                                          sizeof(struct task_struct),
                                          _Alignof(struct task_struct),
                                          NULL,
                                          NULL,
                                          GFP_ATOMIC);
}

// TODO: PID managment should be it's own subsystem
pid_t get_pid() {
    size_t pid = 1;
    while (bm_get(pid_map.bitmap, pid)) { pid++; }
    bm_set(pid_map.bitmap, pid);
    return pid;
}

void free_pid(pid_t pid) {
    bm_clear(pid_map.bitmap, pid);
}

struct task_struct *task_alloc(const char *name) {
    struct task_struct *task = kmem_cache_alloc(task_struct_cache, 0);
    if (!task) { return NULL; }

    memset(task, 0, sizeof(struct task_struct));
    task->stack = __va(pm_get_zeroed_page(GFP_ATOMIC));
    task->pid   = get_pid();

    if (name) {
        strncpy(task->name, name, TASK_NAME_LEN - 1);
        task->name[TASK_NAME_LEN - 1] = '\0';
    }

    task->state = TASK_NEW;
    list_add_tail(&task->task_list, &task_list);

    return task;
}

void task_free(struct task_struct *task) {
    pm_free_page((phys_addr_t) __pa(task->stack));
    free_pid(task->pid);
    list_del(&task->task_list);
    kmem_cache_free(task_struct_cache, task);
}

void task_make_runnable(struct task_struct *task) {
    list_add_tail(&task->list, &runqueue);
    task->state = RUNNABLE;
}

void __noreturn task_exit() {
    struct task_struct *task = get_current_task();
    list_del(&task->list);
    task->state = DEAD;
    schedule();

    while (1);
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

    return &idle_task;
}

struct task_struct *get_current_task() {
    return current;
}

extern void context_switch(struct task_struct *prev, struct task_struct *next);

void schedule() {
    struct task_struct *cur  = get_current_task();
    struct task_struct *next = pick_next();

    if (next == cur) { return; }

    need_reschedule = 0;

    if (cur->state == RUNNING) { cur->state = RUNNABLE; }
    next->state = RUNNING;

    current = next;

    context_switch(cur, next);
}

void scheduler_tick() {
    need_reschedule = 1;
}

int need_resched() {
    return need_reschedule;
}
