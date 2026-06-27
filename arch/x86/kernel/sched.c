#include <asi/tss.h>
#include <nyx/proc.h>

void arch_schedule_tail(struct thread *prev, struct thread *next) {
    (void) prev;
    default_tss.rsp[0] = (u64) next->kstack + PAGE_SIZE;
}
