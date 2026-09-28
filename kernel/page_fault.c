#include <mm/address.h>
#include <mm/physmem.h>
#include <mm/virtmem.h>
#include <nyx/errno.h>
#include <nyx/kernel.h>
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

DEFINE_SUBSYS_LOG(pgflt_log, "pgflt", CONFIG_PGFLT_LOG_LEVEL);

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
                                 struct vm_map_entry    *vme,
                                 const struct vm_pginfo *pginfo) {
    virt_addr_t    va = PG_ALIGN_DN(pfi->pf_addr);
    vm_sizeclass_t sc;
    struct page   *old, *new;
    int            res;

    old = phys_to_page(pginfo->pi_phys_base);
    if (refcount_get(&old->pg_refcnt) == 1) { return vm_set_page_prot(vms->v_pgd, va, vme->vm_prot, &sc); }

    new = vm_alloc_page(M_SLEEPOK);
    if (!new) { panic("oom"); }

    memcpy(page_address(new), page_address(old), PAGE_SIZE);

    res = vm_remap_page(vms->v_pgd, page_to_phys(new), va, vme->vm_prot);
    if (res) {
        vm_put_page(new);
        return -EFAULT;
    }

    vm_put_page(old);
    return 0;
}

static int handle_anon_page_fault(struct vmspace *vms, struct pgflt_info *pfi, struct vm_map_entry *vma) {
    virt_addr_t  va = PG_ALIGN_DN(pfi->pf_addr);
    struct page *pg;
    int          res;

    pr_debug(pgflt_log, "    handling anonymous page fault\n");

    pg = vm_alloc_page(M_SLEEPOK);

    if ((res = vm_map_page(vms->v_pgd,
                           page_to_phys(pg),
                           va,
                           get_vm_flags(vma) | VM_USER,
                           vm_sc_for_bytes(PAGE_SIZE),
                           M_SLEEPOK))) {
        vm_put_page(pg);
        return res;
    }

    pr_debug(pgflt_log, "    mapped new page %#lx to va %#lx with prot %#lx\n", page_to_phys(pg), va, vma->vm_prot);

    return 0;
}

static int handle_file_backed_page_fault(struct vmspace *vms, struct pgflt_info *pfi, struct vm_map_entry *vma) {
    struct vattr attr;
    off_t        file_off;
    size_t       rem, to_read;
    virt_addr_t  va = PG_ALIGN_DN(pfi->pf_addr);
    struct page *pg;
    void        *frame;
    int          res;

    pr_debug(pgflt_log, "    handling file backed page fault\n");

    res = VOP_GETATTR(vma->vm_vn, &attr);
    if (res) {
        return res; // SIGBUS
    }

    if (attr.va_type != VREG) { return -EBADF; }

    file_off = vma->vm_foff + PG_ALIGN_DN(pfi->pf_addr - vma->vm_start);
    rem      = (file_off < attr.va_size) ? (attr.va_size - file_off) : 0;
    to_read  = MIN(rem, PAGE_SIZE);

    pg = vm_alloc_page(M_SLEEPOK);
    if (!pg) { panic("oom"); }

    frame = page_address(pg);
    if (to_read > 0) {
        res = vn_rdwr(UIO_READ, vma->vm_vn, frame, to_read, file_off, NULL);
        if (res) { return res; }
    }

    if (to_read < PAGE_SIZE) { memset((char *) frame + to_read, 0, PAGE_SIZE - to_read); }

    if ((res = vm_map_page(vms->v_pgd,
                           page_to_phys(pg),
                           va,
                           get_vm_flags(vma) | VM_USER,
                           vm_sc_for_bytes(PAGE_SIZE),
                           M_SLEEPOK))) {
        pm_free_page(page_to_phys(pg));
        return res;
    }

    pr_debug(pgflt_log, "    mapped new page %#lx to va %#lx with prot %#lx\n", page_to_phys(pg), va, vma->vm_prot);

    return 0;
}

static int handle_user_page_fault(struct thread *t, struct pgflt_info *pfi) {
    struct process      *p = t->t_proc;
    struct vm_map_entry *vma;
    struct vm_pginfo     pginfo;
    int                  res;

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

    res = vm_query_page(p->p_mm->v_pgd, pfi->pf_addr, &pginfo);

    if (pfi->pf_reason == PGFLT_NOT_PRESENT && res == -ENOENT) {
        if (vma->vm_flags & MAP_ANONYMOUS) {
            return handle_anon_page_fault(p->p_mm, pfi, vma);
        } else if (vma->vm_vn) {
            return handle_file_backed_page_fault(p->p_mm, pfi, vma);
        }
    } else if (!res && pfi->pf_reason == PGFLT_PROT_VIOLATION) {
        if (!(pginfo.pi_prot & VM_WRITE) && vma->vm_prot & VM_WRITE) {
            return handle_cow_page_fault(p->p_mm, pfi, vma, &pginfo);
        }
    } else {
        return -EFAULT; // TODO: deliver SIGSEGV
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
