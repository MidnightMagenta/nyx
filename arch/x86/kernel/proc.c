#include <nyx/sched.h>
#include <nyx/types.h>

#include <asi/page.h>
#include <asi/proc.h>
#include <asi/traps.h>

extern void start_task();

void arch_init_task(struct task_struct *task, void *stack, void (*entry)(void *), virt_addr_t context) {
    u64                    *sp = (u64 *) ((char *) stack + PAGE_SIZE);
    struct interrupt_frame *intr_frame;

    sp -= sizeof(*intr_frame) / sizeof(u64);
    intr_frame         = (struct interrupt_frame *) sp;
    intr_frame->ss     = 0x10;                               // HACK: major, usermode - hard coded kernel stack segment
    intr_frame->rsp    = (u64) ((char *) stack + PAGE_SIZE); //
    intr_frame->rflags = 0x202;                              // HACK: major, usermode - hard coded flags
    intr_frame->cs     = 0x08;                               // HACK: major, usermode - hard coded kernel code segment
    intr_frame->rip    = (u64) entry;

    *--sp = (u64) start_task;
    *--sp = 0;             // rbx
    *--sp = 0;             // rbp
    *--sp = (u64) context; // r12
    *--sp = 0;             // r13
    *--sp = 0;             // r14
    *--sp = 0;             // r15

    task->context.rsp = (u64) sp;
}
