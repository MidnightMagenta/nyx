#include <mm/address.h>
#include <mm/physmem.h>
#include <mm/slab.h>
#include <mm/vas.h>
#include <mm/virtmem.h>
#include <nyx/minmax.h>
#include <nyx/string.h>

#include <asi/bug.h>
#include <asi/page.h>

#include <nyx/printk.h>

kmem_cache_t *vas_struct_cache;

extern void map_kernel();
void        vas_init() {
    vas_struct_cache = kmem_create_cache("mm_struct",
                                         sizeof(struct vas_struct),
                                         _Alignof(struct vas_struct),
                                         NULL,
                                         NULL,
                                         GFP_ATOMIC);
    map_kernel();
}

void vas_put(struct vas_struct *vas) {
    int old = refcnt_dec(&vas->user_count);
    BUG_ON(old == 0);

    if (old == 1) {
        atomic_thread_fence(ATOMIC_ACQUIRE);
        // TODO: global, VAS - free userspace
        printk("vas_struct %#p user refcount hit 0\n");
        vas_drop(vas);
    }
}

void vas_drop(struct vas_struct *vas) {
    int old = refcnt_dec(&vas->refcount);
    BUG_ON(old == 0);

    if (old == 1) {
        atomic_thread_fence(ATOMIC_ACQUIRE);
        // TODO: global, VAS - free everything
        printk("vas_struct %#p total refcount hit 0\n");
    }
}

void vas_activate(struct vas_struct *vas) {
    vm_activate(vas->pgd);
}
