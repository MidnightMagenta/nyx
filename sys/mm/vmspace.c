#include <mm/address.h>
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

#include <asi/bug.h>
#include <asi/memory.h>
#include <asi/page.h>

DEFINE_SUBSYS_LOG(vmspace_log, "vmspace", CONFIG_VMSPACE_LOG_LEVEL);

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

static int vm_map_entry_cmp(void *priv, const struct list_head *a, const struct list_head *b) {
    (void) priv;

    struct vm_map_entry *ea = container_of(a, struct vm_map_entry, vm_list);
    struct vm_map_entry *eb = container_of(b, struct vm_map_entry, vm_list);
    if (ea->vm_start < eb->vm_start) { return -1; }
    if (ea->vm_start > eb->vm_start) { return 1; }
    return 0;
}

static inline void __vm_map_insert(struct vmspace *vs, struct vm_map_entry *vme) {
    list_add_tail(&vme->vm_list, &vs->v_vmmap);
    list_sort(NULL, &vs->v_vmmap, vm_map_entry_cmp);
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
    __vm_map_insert(vs, vme);

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

struct vm_map_entry *find_vma(struct process *p, virt_addr_t va) {
    struct vm_map_entry *vma;

    list_for_each_entry(vma, &p->p_mm->v_vmmap, vm_list) {
        if (vma->vm_start <= va && vma->vm_end > va) { return vma; }
    }

    return NULL;
}

struct vmspace *vmspace_fork(struct process *p) {
    struct vmspace      *newvm = vmspace_new(p);
    struct vm_map_entry *parent_vme, *new_vme;
    if (!newvm) { return NULL; }

    list_for_each_entry(parent_vme, &p->p_mm->v_vmmap, vm_list) {
        int res;

        new_vme = vm_map_clone_vme(parent_vme);
        if (!new_vme) { goto fail0; }
        __vm_map_insert(newvm, new_vme);

        res = vm_copy_cow(newvm->v_pgd, p->p_mm->v_pgd, new_vme->vm_start, new_vme->vm_end - new_vme->vm_start);
        if (res) { goto fail0; }
    }

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

    (void) parent;

    refcount_init(&newvm->v_refcount, 1);
    list_init(&newvm->v_vmmap);
    newvm->v_pgd = vm_alloc_page_table(M_SLEEPOK);
    if (!newvm->v_pgd) { goto fail0; }

    if (vm_copy_kernel(newvm->v_pgd)) { goto fail1; }

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

extern void vmspace_unmap_entry(struct vmspace *vm, struct vm_map_entry *vme);

void vmspace_put(struct vmspace *vm) {
    struct vm_map_entry *vme, *tmp;

    if (!vm) { return; }
    if (!refcount_dec_and_test(&vm->v_refcount)) { return; }

    list_for_each_entry_safe(vme, tmp, &vm->v_vmmap, vm_list) {
        vmspace_unmap_entry(vm, vme);
        if (vme->vm_vn) { vput(vme->vm_vn); }
        vm_map_remove(vm, vme);
    }

    vm_free_page_table(vm->v_pgd);
    kmem_cache_free(vmspace_cache, vm);
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
        phys_addr_t  pa    = pm_get_zeroed_page(gfp_flags);
        size_t       chunk = MIN(PAGE_SIZE, len - off);
        struct page *pg    = phys_to_page(pa);

        refcount_inc(&pg->pg_refcnt);

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

    pr_debug(vmspace_log, "mmap(%#p, %#lx, %ld, %#x, %#x, %#p, %ld)\n", vs, addr, len, prot, flags, vp, off);

    if (len == 0) { return -EINVAL; }
    if (off & (PAGE_SIZE - 1)) { return -EINVAL; }
    if (flags & MAP_FIXED && (addr & (PAGE_SIZE - 1))) { return -EINVAL; }

    if (len > SIZE_MAX - (PAGE_SIZE - 1)) { return -EINVAL; }
    addr = PG_ALIGN_DN(addr);
    len  = PG_ALIGN_UP(len);

    if (flags & MAP_FIXED) {
        if ((res = clear_range(vs, addr, len))) { return res; }
    } else {
        addr = find_free_range(vs, addr, len);
        if (!addr) { return -ENOMEM; }
    }

    pr_debug(vmspace_log, "    mapping to address %#lx\n", addr);

    if ((res = vm_map_insert(vs, addr, len, prot, flags, vp, off))) { return res; }
    if (pa) { *pa = addr; }

    return 0;
}

int vms_munmap(struct vmspace *vs, virt_addr_t addr, size_t len) {
    int res;

    pr_debug(vmspace_log, "munmap(%#p, %#lx, %#lx)\n", vs, addr, len);

    if (addr & (PAGE_SIZE - 1)) { return -EINVAL; }
    if (len == 0) { return -EINVAL; }

    if (len > SIZE_MAX - (PAGE_SIZE - 1)) { return -EINVAL; }
    len = PG_ALIGN_UP(len);

    if (addr >= ARCH_USER_END || len > ARCH_USER_END - addr) { return -EINVAL; }

    if ((res = clear_range(vs, addr, len))) { return res; }
    vm_release(vs, addr, len);

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

    pr_debug(vmspace_log, "kern_mmap called for PID %ld\n", pr->p_pid);

    if (!(flags & MAP_ANONYMOUS)) {
        f = fget(pr, fd);
        if (!f) { return -EBADF; }
    }

    return vms_mmap(pr->p_mm, addr, len, prot, flags, f ? getvnode(f) : NULL, off, pa);
}

int kern_munmap(struct process *pr, virt_addr_t addr, size_t len) {
    return vms_munmap(pr->p_mm, addr, len);
}

int sys_mmap(struct thread *t, struct syscall_args *args, register_t *retval) {
    pr_debug(vmspace_log, "mmap syscall from (pid: %d, name: %s)\n", t->t_proc->p_pid, t->t_proc->p_name);
    return kern_mmap(t->t_proc, args->arg1, args->arg2, args->arg3, args->arg4, args->arg5, args->arg6, retval);
}

int sys_munmap(struct thread *t, struct syscall_args *args, register_t *retval) {
    (void) retval;
    pr_debug(vmspace_log, "munmap syscall from (pid: %d, name: %s)\n", t->t_proc->p_pid, t->t_proc->p_name);
    return kern_munmap(t->t_proc, args->arg1, args->arg2);
}

bool access_ok(const void *uptr, size_t len) {
    virt_addr_t u = (virt_addr_t) uptr;
    if (u <= ARCH_USER_START && u + len >= ARCH_USER_END) { return false; }
    return true;
}

int copyinstr(void *kaddr, const void *uaddr, size_t len, size_t *done) {
    virt_addr_t u = (virt_addr_t) uaddr;

    if (u >= ARCH_USER_END) { return -EFAULT; }

    size_t  room = ARCH_USER_END - u;
    size_t  n    = len > room ? room : len;
    ssize_t r    = __copyinstr(kaddr, uaddr, n);
    if (r < 0) { return r; }
    if (r == 0) { return len > room ? -EFAULT : -ENAMETOOLONG; }
    if (done) { *done = r; }
    return 0;
}
