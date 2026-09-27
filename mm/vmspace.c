#include <mm/mm_types.h>
#include <mm/physmem.h>
#include <mm/slab.h>
#include <mm/virtmem.h>
#include <mm/vmspace.h>
#include <nyx/errno.h>
#include <nyx/list.h>
#include <nyx/minmax.h>
#include <nyx/proc.h>
#include <nyx/refcount.h>
#include <nyx/string.h>
#include <nyx/syscall.h>
#include <nyx/types.h>
#include <uapi/mman.h>

#include <asi/address.h>
#include <asi/bug.h>
#include <asi/memory.h>
#include <asi/page.h>

#define pr_fmt(fmt) "vmspace: " fmt

kmem_cache_t *vmspace_cache;
kmem_cache_t *vmmap_cache;

void vmspace_init() {
    vmspace_cache =
            kmem_create_cache("vmspace", sizeof(struct vmspace), _Alignof(struct vmspace), NULL, NULL, M_SLEEPOK);
    vmmap_cache = kmem_create_cache("vm_map_entry",
                                    sizeof(struct vm_map_entry),
                                    _Alignof(struct vm_map_entry),
                                    NULL,
                                    NULL,
                                    0);
}

struct vmspace *vmspace_fork(struct process *p) {
    struct vmspace *newvm = vmspace_new(p);
    if (!newvm) { return NULL; }
    if (vm_copy_user(newvm->v_pgd, p->p_mm->v_pgd, M_SLEEPOK) != 0) { goto fail0; }

    return newvm;

fail0:
    vmspace_put(newvm);
    return NULL;
}

struct vmspace *vmspace_share(struct process *parent) {
    refcount_inc(&parent->p_mm->v_refcount);
    return parent->p_mm;
}

struct vmspace *vmspace_new(struct process *parent) {
    struct vmspace *newvm = kmem_cache_alloc(vmspace_cache, M_SLEEPOK);
    if (!newvm) { return NULL; }

    refcount_init(&newvm->v_refcount, 1);
    list_init(&newvm->v_vmmap);
    newvm->v_pgd = vm_alloc_page_table(M_SLEEPOK);
    if (!newvm->v_pgd) { goto fail0; }

    if (vm_copy_kernel(newvm->v_pgd, parent->p_mm->v_pgd)) { goto fail1; }

    return newvm;

fail1:
    vm_free_page_table(newvm->v_pgd);
fail0:
    kmem_cache_free(vmspace_cache, newvm);
    return NULL;
}

void vmspace_activate(struct vmspace *mm) {
    vm_activate(mm->v_pgd);
}

void vmspace_put(struct vmspace *mm) {
    if (!mm) { return; }
    if (refcount_get_dec(&mm->v_refcount) == 1) {
        vm_free_user(mm->v_pgd);
        vm_free_page_table(mm->v_pgd);
        kmem_cache_free(vmspace_cache, mm);
    }
}

int vmspace_map(struct vmspace *mm, virt_addr_t addr, size_t len, unsigned long flags, int gfp_flags) {
    int res;
    addr = PG_ALIGN_DN(addr);
    len  = PG_ALIGN_UP(len);

    for (size_t off = 0; off < len; off += PAGE_SIZE) {
        phys_addr_t pa = pm_get_zeroed_page(gfp_flags);
        if (pa == INVALID_PHYS_ADDR) {
            vm_unmap(mm->v_pgd, addr, len);
            return -ENOMEM;
        }
        if ((res = vm_map(mm->v_pgd, pa, addr + off, PAGE_SIZE, flags | VM_USER, gfp_flags))) {
            pm_free_page(pa);
            vm_unmap(mm->v_pgd, addr, len);
            return res;
        }
    }

    return 0;
}

int vmspace_mapcopy(struct vmspace *mm, virt_addr_t addr, void *data, size_t len, unsigned long flags, int gfp_flags) {
    int    res;
    size_t end = PG_ALIGN_UP(addr + len);
    addr       = PG_ALIGN_DN(addr);

    for (size_t off = 0; addr + off < end; off += PAGE_SIZE) {
        phys_addr_t pa    = pm_get_zeroed_page(gfp_flags);
        size_t      chunk = MIN(PAGE_SIZE, len - off);

        if (pa == INVALID_PHYS_ADDR) {
            vm_unmap(mm->v_pgd, addr, off);
            return -ENOMEM;
        }

        memcpy(__va(pa), (char *) data + off, chunk);
        if (chunk < PAGE_SIZE) memset((char *) __va(pa) + chunk, 0, PAGE_SIZE - chunk);

        if ((res = vm_map(mm->v_pgd, pa, addr + off, PAGE_SIZE, flags | VM_USER, gfp_flags))) {
            pm_free_page(pa);
            vm_unmap(mm->v_pgd, addr, off);
            return res;
        }
    }
    return 0;
}

void vmspace_unmap(struct vmspace *mm, virt_addr_t addr, size_t len) {
    vm_unmap(mm->v_pgd, addr, len);
}

static int vm_map_entry_cmp(void *priv, const struct list_head *a, const struct list_head *b) {
    (void) priv;

    struct vm_map_entry *ea = container_of(a, struct vm_map_entry, vm_list);
    struct vm_map_entry *eb = container_of(b, struct vm_map_entry, vm_list);
    if (ea->vm_start < eb->vm_start) { return -1; }
    if (ea->vm_start > eb->vm_start) { return 1; }
    return 0;
}

static inline int vm_map_insert(struct vmspace *vs,
                                virt_addr_t     addr,
                                size_t          len,
                                int             prot,
                                int             flags,
                                struct vnode   *vn,
                                off_t           foff) {
    struct vm_map_entry *vme = kmem_cache_alloc(vmmap_cache, M_SLEEPOK);
    if (!vme) { return -ENOMEM; }

    vme->vm_start = addr;
    vme->vm_end   = addr + len;
    vme->vm_prot  = prot;
    vme->vm_flags = flags;
    vme->vm_vn    = vn;
    vme->vm_foff  = foff;

    list_init(&vme->vm_list);
    list_add_tail(&vme->vm_list, &vs->v_vmmap);
    list_sort(NULL, &vs->v_vmmap, vm_map_entry_cmp);

    return 0;
}

static inline void vm_map_remove(struct vmspace *vs, struct vm_map_entry *e) {
#ifdef __DEBUG
    struct vm_map_entry *ent;
    bool                 is_valid = false;
    list_for_each_entry(ent, &vs->v_vmmap, vm_list) {
        if (ent == e) {
            is_valid = true;
            return;
        }
    }
    if (!is_valid) { BUG(); }
#endif

    list_del(&e->vm_list);
    kmem_cache_free(vmmap_cache, e);
}

static inline struct vm_map_entry *vm_map_clone_vme(struct vm_map_entry *e) {
    struct vm_map_entry *new_vme = kmem_cache_alloc(vmmap_cache, M_SLEEPOK);
    if (!new_vme) { return NULL; }

    memcpy(new_vme, e, sizeof(struct vm_map_entry));
    list_init(&new_vme->vm_list);

    return new_vme;
}

static inline virt_addr_t find_free_range(struct vmspace *vs, virt_addr_t hint, size_t len) {
    struct vm_map_entry *vme;
    virt_addr_t          cursor = hint ? PG_ALIGN_DN(hint) : ARCH_USER_START;
    if (cursor < ARCH_USER_START) { cursor = ARCH_USER_START; }

    list_for_each_entry(vme, &vs->v_vmmap, vm_list) {
        if (vme->vm_end <= cursor) { continue; }
        if (vme->vm_start >= cursor + len) { break; }

        cursor = vme->vm_end;
    }

    if (cursor + len <= ARCH_USER_END) { return cursor; }

    if (hint) { return find_free_range(vs, 0, len); }

    return 0;
}

static inline int clear_range(struct vmspace *vs, virt_addr_t addr, size_t len) {
    virt_addr_t          end = addr + len;
    struct vm_map_entry *e, *tmp;

    list_for_each_entry_safe(e, tmp, &vs->v_vmmap, vm_list) {
        if (e->vm_end <= addr) { continue; }
        if (e->vm_start >= end) { break; }

        if (e->vm_start < addr && e->vm_end > end) {
            struct vm_map_entry *right = vm_map_clone_vme(e);
            if (!right) { return -ENOMEM; }
            right->vm_start = end;
            e->vm_end       = addr;
            list_add(&right->vm_list, &e->vm_list);
            break;
        }
        if (e->vm_start < addr) {
            e->vm_end = addr;
        } else if (e->vm_end > end) {
            e->vm_start = end;
        } else {
            vm_map_remove(vs, e);
        }
    }

    return 0;
}

int vms_mmap(struct vmspace *vs,
             virt_addr_t     addr,
             size_t          len,
             int             prot,
             int             flags,
             struct vnode   *vp,
             off_t           off,
             virt_addr_t    *pa) {
    int res;

    if (len == 0) { return -EINVAL; }
    addr = PG_ALIGN_DN(addr);
    len  = PG_ALIGN_UP(len);

    if (flags & MAP_FIXED) {
        if ((res = clear_range(vs, addr, len))) { return res; }
    } else {
        addr = find_free_range(vs, addr, len);
        if (!addr) { return -ENOMEM; }
    }

    if ((res = vm_map_insert(vs, addr, len, prot, flags, vp, off))) { return res; }
    *pa = addr;

    return 0;
}

int vms_munmap(struct vmspace *vs, virt_addr_t addr, size_t len) {
    return 0;
}

int kern_mmap(struct process *pr,
              virt_addr_t     addr,
              size_t          len,
              int             prot,
              int             flags,
              int             fd,
              off_t           off,
              virt_addr_t    *pa) {
    struct file *f = NULL;

    if (!(flags & MAP_ANONYMOUS)) {
        f = fget(pr, fd);
        if (!f) { return -EBADF; }
    }

    return vms_mmap(pr->p_mm, addr, len, prot, flags, f ? getvnode(f) : NULL, off, pa);
}

int kern_munmap(struct process *pr, virt_addr_t addr, size_t len);

int sys_mmap(struct thread *t, struct syscall_args *args, register_t *retval) {
    return kern_mmap(t->t_proc, args->arg1, args->arg2, args->arg3, args->arg4, args->arg5, args->arg6, retval);
}
