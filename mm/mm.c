#include <mm/address.h>
#include <mm/mm.h>
#include <mm/physmem.h>
#include <mm/slab.h>
#include <mm/virtmem.h>
#include <nyx/minmax.h>
#include <nyx/string.h>

#include <asi/page.h>

kmem_cache_t           *mm_struct_cache;
extern struct mm_struct init_mm;

void mm_init() {
    mm_struct_cache = kmem_create_cache("mm_struct",
                                        sizeof(struct mm_struct),
                                        _Alignof(struct mm_struct),
                                        NULL,
                                        NULL,
                                        GFP_ATOMIC);
}

extern void __mm_copy_higher_half(struct mm_struct *dest, struct mm_struct *src);

struct mm_struct *mm_alloc() {
    struct mm_struct *mm = kmem_cache_alloc(mm_struct_cache, 0);

    mm->pgd = vm_get_page_table(GFP_ATOMIC);

    __mm_copy_higher_half(mm, &init_mm);

    atomic_set(&mm->refcount, 1);
    return mm;
}

void mm_map(struct mm_struct *mm, virt_addr_t vaddr, size_t size, u32 flags, int gfp_flags) {
    for (size_t off = 0; off < size; off += PAGE_SIZE) {
        phys_addr_t pa = pm_get_zeroed_page(gfp_flags);
        vm_map(mm->pgd, pa, vaddr + off, PAGE_SIZE, flags, gfp_flags);
    }
}

void mm_map_copy(struct mm_struct *mm, virt_addr_t vaddr, const void *data, size_t size, u32 flags, int gfp_flags) {
    for (size_t off = 0; off < size; off += PAGE_SIZE) {
        phys_addr_t pa    = pm_get_zeroed_page(gfp_flags);
        size_t      chunk = MIN(PAGE_SIZE, size - off);

        memcpy(__va(pa), (char *) data + off, chunk);
        if (chunk < PAGE_SIZE) { memset(((char *) __va(pa)) + chunk, 0, PAGE_SIZE - chunk); }

        vm_map(mm->pgd, pa, vaddr + off, PAGE_SIZE, flags | VM_USER, gfp_flags);
    }
}

void mm_free(struct mm_struct *mm) {
    vm_free_page_table(mm->pgd);
    kmem_cache_free(mm_struct_cache, mm);
}
