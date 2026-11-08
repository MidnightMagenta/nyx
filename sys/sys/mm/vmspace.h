#ifndef _MM_MM_H
#define _MM_MM_H

#include <mm/mm_types.h>
#include <mm/virtmem.h>
#include <nyx/atomic.h>
#include <nyx/current.h>
#include <nyx/errno.h>
#include <nyx/proc.h>
#include <uapi/posix_types.h>

#include <asi/access_user.h>

struct vnode;

struct vmspace *vmspace_fork(struct process *p);
struct vmspace *vmspace_share(struct process *p);
struct vmspace *vmspace_new(struct process *parent);
void            vmspace_activate(struct vmspace *mm);
void            vmspace_put(struct vmspace *mm);

int  vmspace_map(struct vmspace *mm, virt_addr_t addr, size_t len, unsigned long flags, int gfp_flags);
int  vmspace_mapcopy(struct vmspace *mm, virt_addr_t addr, void *data, size_t len, unsigned long flags, int gfp_flags);
void vmspace_unmap(struct vmspace *mm, virt_addr_t addr, size_t len);

int vms_mmap(struct vmspace *vs,
             virt_addr_t     addr,
             size_t          len,
             int             prot,
             int             flags,
             struct vnode   *vp,
             off_t           off,
             virt_addr_t    *pa);
int vms_munmap(struct vmspace *vs, virt_addr_t addr, size_t len);

int kern_mmap(struct process *pr,
              virt_addr_t     addr,
              size_t          len,
              int             prot,
              int             flags,
              int             fd,
              off_t           off,
              virt_addr_t    *pa);
int kern_munmap(struct process *pr, virt_addr_t addr, size_t len);

bool access_ok(const void *uptr, size_t len);

static inline int copyout(void *uaddr, const void *kaddr, size_t len) {
    if (!access_ok(uaddr, len)) { return -EFAULT; }
    return __copy_user(uaddr, kaddr, len);
}

static inline int copyin(void *kaddr, const void *uaddr, size_t len) {
    if (!access_ok(uaddr, len)) { return -EFAULT; }
    return __copy_user(kaddr, uaddr, len);
}

int copyinstr(void *kaddr, const void *uaddr, size_t len, size_t *done);

#endif
