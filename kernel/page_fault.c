#include <mm/address.h>
#include <mm/physmem.h>
#include <mm/virtmem.h>
#include <nyx/errno.h>
#include <nyx/minmax.h>
#include <nyx/page_fault.h>
#include <nyx/panic.h>
#include <nyx/proc.h>
#include <nyx/stddef.h>
#include <nyx/string.h>
#include <nyx/types.h>
#include <nyx/vfs.h>

#include <uapi/mman.h>

#include <asi/page.h>

static struct vm_map_entry *find_faulting_vma(struct process *p, virt_addr_t va) {
    struct vm_map_entry *vma;

    list_for_each_entry(vma, &p->p_mm->v_vmmap, vm_list) {
        if (vma->vm_start <= va && vma->vm_end > va) { return vma; }
    }

    return NULL;
}

static inline unsigned long get_vm_flags(struct vm_map_entry *vma) {
    unsigned long flags = 0;
    if (vma->vm_prot & PROT_EXEC) { flags |= VM_EXEC; }
    if (vma->vm_prot & PROT_READ) { flags |= VM_READ; }
    if (vma->vm_prot & PROT_WRITE) { flags |= VM_WRITE; }
    return flags;
}

static int handle_cow_page_fault(struct vmspace         *vms,
                                 struct pgflt_info      *pfi,
                                 struct vm_map_entry    *vma,
                                 const struct vm_pginfo *pginfo) {
    panic("COW page at addr %#p", pfi->pf_addr);
}

static int handle_anon_page_fault(struct vmspace *vms, struct pgflt_info *pfi, struct vm_map_entry *vma) {
    struct vm_pginfo pginfo;
    phys_addr_t      pa;
    virt_addr_t      va;
    struct page     *pg;
    int              res;

    res = vm_query_page(vms->v_pgd, pfi->pf_addr, &pginfo);

    if (res == -ENOENT) {
        pa = pm_get_zeroed_page(M_SLEEPOK);
        if (pa == INVALID_PHYS_ADDR) { panic("oom"); }

        pg = phys_to_page(pa);
        va = PG_ALIGN_DN(pfi->pf_addr);

        if ((res = vm_map_page(vms->v_pgd,
                               pa,
                               va,
                               get_vm_flags(vma) | VM_USER,
                               vm_sc_for_bytes(PAGE_SIZE),
                               M_SLEEPOK))) {
            pm_free_page(pa);
            return res;
        }

        refcount_inc(&pg->pg_refcnt);
    } else if (res == 0 && pfi->pf_write && !(pginfo.pi_flags & VM_WRITE)) {
        return handle_cow_page_fault(vms, pfi, vma, &pginfo);
    } else {
        return res;
    }

    return 0;
}

static int handle_file_backed_page_fault(struct vmspace *vms, struct pgflt_info *pfi, struct vm_map_entry *vma) {
    struct vattr attr;
    off_t        file_off;
    size_t       rem, to_read;
    phys_addr_t  pa;
    virt_addr_t  va;
    struct page *pg;
    void        *frame;
    int          res;

    res = VOP_GETATTR(vma->vm_vn, &attr);
    if (res) {
        return res; // SIGBUS
    }

    if (attr.va_type != VREG) { return -EBADF; }

    file_off = vma->vm_foff + PG_ALIGN_DN(pfi->pf_addr - vma->vm_start);
    rem      = (file_off < attr.va_size) ? (attr.va_size - file_off) : 0;
    to_read  = MIN(rem, PAGE_SIZE);

    pa = pm_get_zeroed_page(M_SLEEPOK);
    if (pa == INVALID_PHYS_ADDR) { panic("oom"); }
    pg = phys_to_page(pa);
    va = PG_ALIGN_DN(pfi->pf_addr);
    refcount_inc(&pg->pg_refcnt);

    frame = __va(pa);
    if (to_read > 0) {
        res = vn_rdwr(UIO_READ, vma->vm_vn, frame, to_read, file_off, NULL);
        if (res) { return res; }
    }

    if (to_read < PAGE_SIZE) { memset((char *) frame + to_read, 0, PAGE_SIZE - to_read); }

    if ((res = vm_map_page(vms->v_pgd, pa, va, get_vm_flags(vma) | VM_USER, vm_sc_for_bytes(PAGE_SIZE), M_SLEEPOK))) {
        pm_free_page(pa);
        return res;
    }

    return 0;
}

static int handle_user_page_fault(struct thread *t, struct pgflt_info *pfi) {
    struct process      *p = t->t_proc;
    struct vm_map_entry *vma;

    if (pfi->pf_reason == PGFLT_RESERVED_BIT) {
        panic("Attempted to access page with reserved bit set at address %#p", pfi->pf_addr);
    }

    vma = find_faulting_vma(p, pfi->pf_addr);
    if (!vma) {
        return -EFAULT;
    } // TODO: deliver SIGSEGV
    else if (pfi->pf_write && !(vma->vm_prot & PROT_WRITE)) {
        return -EFAULT;
    } // TODO: deliver SIGSEGV
    else if (pfi->pf_exec && !(vma->vm_prot & PROT_EXEC)) {
        return -EFAULT;
    } // TODO: deliver SIGSEGV
    else if (!(vma->vm_prot & PROT_READ)) {
        return -EFAULT;
    } // TODO: deliver SIGSEGV
    else if (vma->vm_prot & PROT_NONE) {
        return -EFAULT;
    }

    if (pfi->pf_reason == PGFLT_NOT_PRESENT) {
        if (vma->vm_flags & MAP_ANONYMOUS) {
            return handle_anon_page_fault(p->p_mm, pfi, vma);
        } else if (vma->vm_vn) {
            return handle_file_backed_page_fault(p->p_mm, pfi, vma);
        }
    }

    panic("unhandled page fault at %#p", pfi->pf_addr);
}

int handle_page_fault(struct thread *t, struct pgflt_info *pfi) {
    if (pfi->pf_user) {
        return handle_user_page_fault(t, pfi);
    } else {
        panic("kernel page fault at address %#p", pfi->pf_addr);
    }
}
