#include <mm/address.h>
#include <mm/physmem.h>
#include <mm/slab.h>
#include <mm/virtmem.h>
#include <mm/vmspace.h>
#include <nyx/minmax.h>
#include <nyx/string.h>

#include <asi/bug.h>
#include <asi/page.h>

#include <nyx/printk.h>

#define pr_fmt(fmt) "vas: " fmt

kmem_cache_t *vmspace_cache;

void vas_init() {
    vmspace_cache =
            kmem_create_cache("vmspace", sizeof(struct vmspace), _Alignof(struct vmspace), NULL, NULL, M_NOSLEEP);
}

struct vmspace *vmspace_fork(struct process *p) {
    struct vmspace *newvm = kmem_cache_alloc(vmspace_cache, M_SLEEPOK);

    if (!newvm) { return NULL; }
    refcount_init(&newvm->refcount, 1);
    list_init(&newvm->vma_regions);
    newvm->pgd = vm_get_page_table(M_SLEEPOK);
    if (!newvm->pgd) { goto fail0; }
    if (vm_copy(newvm->pgd, p->mm->pgd, M_SLEEPOK) != 0) { goto fail1; }

    return newvm;

fail1:
    vm_free_page_table(newvm->pgd);
fail0:
    kmem_cache_free(vmspace_cache, newvm);
    return NULL;
}

struct vmspace *vmspace_share(struct process *parent) {
    refcount_inc(&parent->mm->refcount);
    return parent->mm;
}

void vmspace_put(struct vmspace *mm) {
    if (!mm) { return; }
    if (refcount_get_dec(&mm->refcount) == 1) { vm_free_user(mm->pgd); }
}
