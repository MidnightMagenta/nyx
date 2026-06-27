#ifndef _MM_MM_H
#define _MM_MM_H

#include <mm/mm_types.h>
#include <mm/virtmem.h>
#include <nyx/atomic.h>
#include <nyx/proc.h>

struct vmspace *vmspace_fork(struct process *p);
struct vmspace *vmspace_share(struct process *p);
struct vmspace *vmspace_new(struct process *parent);
void            vmspace_activate(struct vmspace *mm);
void            vmspace_put(struct vmspace *mm);
void            vmspace_map(struct vmspace *mm, virt_addr_t addr, size_t len, unsigned long flags, int gfp_flags);
void vmspace_mapcopy(struct vmspace *mm, virt_addr_t addr, void *data, size_t len, unsigned long flags, int gfp_flags);
void vmspace_unmap(struct vmspace *mm, virt_addr_t addr, size_t len);

static inline int copyout(struct vmspace *mm, virt_addr_t dstva, char *src, size_t len) {
    return vm_copyout(mm->pgd, dstva, src, len);
}

static inline int compyin(struct vmspace *mm, char *dst, virt_addr_t srcva, size_t len) {
    return vm_copyin(mm->pgd, dst, srcva, len);
}

#endif
