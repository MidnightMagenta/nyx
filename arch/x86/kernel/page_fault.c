#include <nyx/current.h>
#include <nyx/kernel.h>
#include <nyx/page_fault.h>
#include <nyx/panic.h>
#include <nyx/printk.h>
#include <nyx/proc.h>

#include <asi/traps.h>

DECLARE_SUBSYS_LOG(pgflt_log);

#define FAULT_PRESENT      (1 << 0)
#define FAULT_WRITE        (1 << 1)
#define FAULT_USER         (1 << 2)
#define FAULT_RESERVED_BIT (1 << 3)
#define FAULT_INSTR_FETCH  (1 << 4)

static inline enum pgflt_reason decode_fault_reason(u64 ecode) {
    if (ecode & FAULT_INSTR_FETCH) { return PGFLT_INSTR_FETCH_NX; }
    if (ecode & FAULT_RESERVED_BIT) { return PGFLT_RESERVED_BIT; }
    return (ecode & FAULT_PRESENT) ? PGFLT_PROT_VIOLATION : PGFLT_NOT_PRESENT;
}

static inline u64 read_cr2() {
    u64 cr2;
    asm volatile("mov %%cr2, %0"
                 : "=r"(cr2)::"memory");

    return cr2;
}

extern int handle_page_fault(struct thread *, struct pgflt_info *);

void page_fault_handler(struct trap_frame *frame) {
    struct pgflt_info pfi = {
            .pf_addr   = read_cr2(),
            .pf_reason = decode_fault_reason(frame->ecode),
            .pf_user   = frame->ecode & FAULT_USER,
            .pf_write  = frame->ecode & FAULT_WRITE,
            .pf_exec   = frame->ecode & FAULT_INSTR_FETCH,
    };

    pr_debug(pgflt_log, "page fault at address %#p\n", pfi.pf_addr);
    pr_debug(pgflt_log,
             "    x86 error code: [%s %s %s %s %s]\n",
             frame->ecode & FAULT_PRESENT ? "A" : "P",
             frame->ecode & FAULT_WRITE ? "W" : "R",
             frame->ecode & FAULT_USER ? "U" : "S",
             frame->ecode & FAULT_INSTR_FETCH ? "IF" : "AC",
             frame->ecode & FAULT_RESERVED_BIT ? "RB" : "NRB");

    int res = handle_page_fault(current(), &pfi);

    if (res) { panic("Page fault not handled %d\n", res); }
}
