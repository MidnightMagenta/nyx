#include <nyx/compiler.h>
#include <nyx/printk.h>
#include <nyx/types.h>

#include <asi/irq.h>
#include <asi/traps.h>

static void print_cr() {
    u64 cr0 = 0, cr2 = 0, cr3 = 0, cr4 = 0, cr8 = 0;
    __asm__ volatile("mov %%cr0, %0;"
                     "mov %%cr2, %1;"
                     "mov %%cr3, %2;"
                     "mov %%cr4, %3;"
                     "mov %%cr8, %4"
                     : "=r"(cr0), "=r"(cr2), "=r"(cr3), "=r"(cr4), "=r"(cr8)
                     :
                     : "memory");

    printk("cr0: 0x%016lx, cr2: 0x%016lx, cr3: 0x%016lx, cr4: 0x%016lx, cr8: "
           "0x%016lx\n",
           cr0,
           cr2,
           cr3,
           cr4,
           cr8);
}

static void __noreturn isr_log_and_die(struct trap_frame *frame) {
    printk("interrupt %d at address 0x%016lx\n", frame->vector, frame->rip);
    printk("cs: 0x%016lx, ss: 0x%016lx, rflags: 0x%016lx, ecode: 0x%016lx\n",
           frame->cs,
           frame->ss,
           frame->rflags,
           frame->ecode);
    printk("registers:\n");
    printk("rax: 0x%016lx, rbx: 0x%016lx, rcx: 0x%016lx, rdx: 0x%016lx\n",
           frame->regs.rax,
           frame->regs.rbx,
           frame->regs.rcx,
           frame->regs.rdx);
    printk("rsp: 0x%016lx, rbp: 0x%016lx, rdi: 0x%016lx, rsi: 0x%016lx\n",
           frame->rsp,
           frame->regs.rbp,
           frame->regs.rdi,
           frame->regs.rsi);
    printk("r8:  0x%016lx, r9:  0x%016lx, r10: 0x%016lx, r11: 0x%016lx\n",
           frame->regs.r8,
           frame->regs.r9,
           frame->regs.r10,
           frame->regs.r11);
    printk("r12: 0x%016lx, r13: 0x%016lx, r14: 0x%016lx, r15: 0x%016lx\n",
           frame->regs.r12,
           frame->regs.r13,
           frame->regs.r14,
           frame->regs.r15);
    print_cr();

    while (1) { asm volatile("hlt"); }
}

void __noreturn unhandled_exception_handler(struct trap_frame *frame) {
    isr_log_and_die(frame);
}

extern void irq_dispatch(unsigned int irq);

void __irq_handler(struct trap_frame *frame) {
    irq_dispatch(vector_to_irq(frame->vector));
}
