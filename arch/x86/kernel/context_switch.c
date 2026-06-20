#include <mm/virtmem.h>
#include <mm/vmspace.h>
#include <nyx/proc.h>
#include <nyx/sched.h>
#include <nyx/stddef.h>

extern void switch_to(struct thread *prev, struct thread *next);

void context_switch(struct thread *prev, struct thread *next) {
    if (prev->proc->mm != next->proc->mm) { vm_activate(next->proc->mm->pgd); }

    switch_to(prev, next);
}
