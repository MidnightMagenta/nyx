#include <nyx/page_fault.h>
#include <nyx/panic.h>
#include <nyx/proc.h>

int handle_page_fault(struct thread *t, struct pgflt_info *pfi) {
    (void) t;
    panic("page fault %#p", pfi->pf_addr);
}
