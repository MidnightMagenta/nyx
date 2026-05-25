#include <mm/mm_types.h>
#include <nyx/proc.h>

extern char init_stack_top[];

struct vas_struct init_vas = {
        .pgd              = NULL,
        .vma_regions      = LIST_HEAD_INIT(init_vas.vma_regions),
        .refcount.__val   = 1,
        .user_count.__val = 1,
};

struct task_struct idle_task = {
        .list       = (struct list_head) LIST_HEAD_INIT(idle_task.list),
        .state      = RUNNABLE,
        .context    = {0},
        .vas        = &init_vas,
        .active_vas = &init_vas,
        .pid        = 0,
        .stack      = init_stack_top,

        .task_list = LIST_HEAD_INIT(idle_task.task_list),
        .name      = "idle_task",
};
