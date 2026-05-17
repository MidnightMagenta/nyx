#include <nyx/sched.h>
#include <nyx/stddef.h>

#include <asi/proc.h>

#define ASM_OFFSET(name, st, member) __asm__ volatile("\n->" #name " %0" ::"i"(offsetof(st, member)))

void __asm_offsets_dummy_fn() {
    ASM_OFFSET(TASK_CONTEXT_RSP, struct task_struct, context.rsp);
}
