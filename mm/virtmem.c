#include <mm/virtmem.h>
#include <nyx/compiler.h>
#include <nyx/errno.h>
#include <nyx/kernel.h>
#include <nyx/minmax.h>

#include <asi/bug.h>
#include <asi/page.h>

DEFINE_SUBSYS_LOG(virtmem_log, "virtmem", CONFIG_VIRTMEM_LOG_LEVEL);

int vm_map(pgd_t *pgd, phys_addr_t pa, virt_addr_t va, size_t len, unsigned long flags, int gfp_flags) {
    size_t         mapped = 0;
    vm_sizeclass_t sc;
    int            res;

    while (mapped < len) {
        sc = vm_pick_sizeclass(va + mapped, pa + mapped, len - mapped, 0);
        if (sc < 0) {
            res = -EINVAL;
            goto fail;
        }

        if ((res = vm_map_page(pgd, pa + mapped, va + mapped, flags, sc, gfp_flags))) { goto fail; }

        mapped += vm_sc_bytes(sc);
    }

    return 0;

fail:
    vm_unmap(pgd, va, mapped);
    return res;
}

int vm_remap(pgd_t *pgd, phys_addr_t pa, virt_addr_t va, size_t len, unsigned long flags, int gfp_flags) {
    BUG();
    __unreachable;
    // unimplemented
}

int vm_unmap(pgd_t *pgd, virt_addr_t va, size_t len) {
    virt_addr_t end = va + len;
    int         res;

    BUG_ON(va & (PAGE_SIZE - 1));
    BUG_ON(len & (PAGE_SIZE - 1));

    while (va < len) {
        vm_sizeclass_t   sc;
        size_t           pgbytes;
        struct vm_pginfo pginfo;

        res = vm_unmap_page(pgd, va, &sc);
        if (res == 0) {
            pgbytes = vm_sc_bytes(sc);
            if (pgbytes > (size_t) (end - va)) { BUG(); }

            va += pgbytes;
            continue;
        }

        if (res == -ENOENT) {
            res = vm_query_page(pgd, va, &pginfo);
            if (res != -ENOENT) { BUG(); }

            va += MIN(pginfo.pi_len, (size_t) (end - va));
            continue;
        }

        return res;
    }

    return 0;
}
