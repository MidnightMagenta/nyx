#include <nyx/sched.h>
#include <nyx/types.h>

#include <asi/page.h>

void arch_init_task(struct task_struct *task, void *stack, void (*entry)(void)) {
    u64 *sp = stack + PAGE_SIZE;

    *--sp = (u64) entry;
    *--sp = 0; // r15
    *--sp = 0; // r14
    *--sp = 0; // r13
    *--sp = 0; // r12
    *--sp = 0; // rbp
    *--sp = 0; // rbx

    task->context.rsp = (u64) sp;
}
