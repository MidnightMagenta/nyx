#include <asi/isr_context.h>
#include <nyx/printk.h>

#define hcf() __asm__("cli; hlt");

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

void __isr_dispatch(struct isr_context *context) {
    // no dispatch for now. Just print what we have and freeze
    printk("interrupt %d at address 0x%016lx\n", context->vector, context->rip);
    printk("cs: 0x%016lx, ss: 0x%016lx, rflags: 0x%016lx, ecode: 0x%016lx\n",
           context->cs,
           context->ss,
           context->rflags,
           context->ecode);
    printk("registers:\n");
    printk("rax: 0x%016lx, rbx: 0x%016lx, rcx: 0x%016lx, rdx: 0x%016lx\n",
           context->regs.rax,
           context->regs.rbx,
           context->regs.rcx,
           context->regs.rdx);
    printk("rsp: 0x%016lx, rbp: 0x%016lx, rdi: 0x%016lx, rsi: 0x%016lx\n",
           context->rsp,
           context->regs.rbx,
           context->regs.rdi,
           context->regs.rsi);
    printk("r8:  0x%016lx, r9:  0x%016lx, r10: 0x%016lx, r11: 0x%016lx\n",
           context->regs.r8,
           context->regs.r9,
           context->regs.r10,
           context->regs.r11);
    printk("r12: 0x%016lx, r13: 0x%016lx, r14: 0x%016lx, r15: 0x%016lx\n",
           context->regs.r12,
           context->regs.r13,
           context->regs.r14,
           context->regs.r15);
    print_cr();

    hcf();
}
