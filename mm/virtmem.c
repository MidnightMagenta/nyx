#include <mm/address.h>
#include <mm/physmem.h>
#include <mm/virtmem.h>
#include <nyx/compiler.h>
#include <nyx/errno.h>
#include <nyx/kernel.h>
#include <nyx/minmax.h>

#include <asi/bug.h>
#include <asi/page.h>

DEFINE_SUBSYS_LOG(virtmem_log, "virtmem", CONFIG_VIRTMEM_LOG_LEVEL);

int vm_map(pgd_t *pgd, phys_addr_t pa, virt_addr_t va, size_t len, unsigned long prot, int gfp_flags) {
    size_t         mapped = 0;
    vm_sizeclass_t sc;
    int            res;

    while (mapped < len) {
        sc = vm_pick_sizeclass(va + mapped, pa + mapped, len - mapped, 0);
        if (sc < 0) {
            res = -EINVAL;
            goto fail;
        }

        if ((res = vm_map_page(pgd, pa + mapped, va + mapped, prot, sc, gfp_flags))) { goto fail; }

        mapped += vm_sc_bytes(sc);
    }

    return 0;

fail:
    vm_unmap(pgd, va, mapped);
    return res;
}

typedef int (*vm_page_op)(pgd_t *pgd, virt_addr_t va, void *arg, vm_sizeclass_t *sc);

struct set_prot_arg {
    unsigned long prot;
};

static int set_prot_op(pgd_t *pgd, virt_addr_t va, void *arg, vm_sizeclass_t *sc) {
    struct set_prot_arg *a = arg;
    return vm_set_page_prot(pgd, va, a->prot, sc);
}

struct copy_cow_arg {
    pgd_t *dst;
};

static inline int copy_cow_op(pgd_t *src, virt_addr_t va, void *arg, vm_sizeclass_t *sc) {
    struct copy_cow_arg *a = arg;
    struct vm_pginfo     info;

    int res = vm_query_page(src, va, &info);
    if (res != 0) { return res; }

    *sc = info.pi_sc;

    page_ref(phys_to_page(info.pi_phys_base));
    res = vm_map_page(a->dst, info.pi_phys_base, va, info.pi_prot & ~VM_WRITE, info.pi_sc, M_SLEEPOK);
    if (res != 0) {
        page_unref(phys_to_page(info.pi_phys_base));
        return res;
    }

    if (info.pi_prot & VM_WRITE) { vm_set_page_prot(src, va, info.pi_prot & ~VM_WRITE, &(vm_sizeclass_t){0}); }
    return 0;
}

static int unmap_op(pgd_t *pgd, virt_addr_t va, void *arg, vm_sizeclass_t *sc) {
    (void) arg;
    return vm_unmap_page(pgd, va, sc);
}

static int release_op(pgd_t *pgd, virt_addr_t va, void *arg, vm_sizeclass_t *sc) {
    (void) arg;
    struct vm_pginfo info;
    struct page     *pg;
    int              res = vm_query_page(pgd, va, &info);
    if (res) return res;

    res = vm_unmap_page(pgd, va, sc);
    if (res) { return res; }

    pg = phys_to_page(info.pi_phys_base);
    if (refcount_dec_and_test(&pg->pg_refcnt)) { __pm_free_pages(pg, pg->pg_head_order); }

    return 0;
}

static int vm_range_walk(pgd_t *pgd, virt_addr_t va, size_t len, vm_page_op op, void *arg) {
    virt_addr_t end = va + len;
    BUG_ON(va & (PAGE_SIZE - 1));
    BUG_ON(len & (PAGE_SIZE - 1));

    while (va < end) {
        vm_sizeclass_t sc;
        int            res = op(pgd, va, arg, &sc);

        if (res == 0) {
            size_t pgbytes = vm_sc_bytes(sc);
            BUG_ON(pgbytes > (size_t) (end - va));
            va += pgbytes;
        } else if (res == -ENOENT) {
            struct vm_pginfo pginfo;
            BUG_ON(vm_query_page(pgd, va, &pginfo) != -ENOENT);
            va += MIN(pginfo.pi_len, (size_t) (end - va));
        } else {
            return res;
        }
    }
    return 0;
}

int vm_set_prot(pgd_t *pgd, virt_addr_t va, size_t len, unsigned long new_prot) {
    struct set_prot_arg a = {.prot = new_prot};
    return vm_range_walk(pgd, va, len, set_prot_op, &a);
}

int vm_copy_cow(pgd_t *dst, pgd_t *src, virt_addr_t va, size_t len) {
    struct copy_cow_arg a = {.dst = dst};
    return vm_range_walk(src, va, len, copy_cow_op, &a);
}

int vm_unmap(pgd_t *pgd, virt_addr_t va, size_t len) {
    return vm_range_walk(pgd, va, len, unmap_op, NULL);
}

void vmspace_unmap_entry(struct vmspace *vm, struct vm_map_entry *vme) {
    vm_range_walk(vm->v_pgd, vme->vm_start, vme->vm_end - vme->vm_start, release_op, NULL);
}

struct page *vm_alloc_page(int gfp_flags) {
    phys_addr_t  pa = pm_get_zeroed_page(gfp_flags);
    struct page *pg;
    if (pa == INVALID_PHYS_ADDR) { return NULL; }

    pg = phys_to_page(pa);
    page_ref(pg);

    return pg;
}

void vm_put_page(struct page *pg) {
    if (refcount_dec_and_test(&pg->pg_refcnt)) { pm_free_page(page_to_phys(pg)); }
}
