#ifndef _NYX_SCHED_H
#define _NYX_SCHED_H

#include <nyx/compiler.h>
#include <nyx/list.h>
#include <nyx/proc.h>
#include <nyx/types.h>

struct task_struct *get_current_task();
void                schedule();

#define yield() schedule()

extern void         arch_init_task(struct task_struct *task, void *stack, void (*entry)(void *), virt_addr_t context);
struct task_struct *task_alloc(const char *name);
void                task_free(struct task_struct *task);
void                task_make_runnable(struct task_struct *task);
void __noreturn     task_exit();

int need_resched();

#endif
