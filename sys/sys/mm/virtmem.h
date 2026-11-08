#ifndef _MM_VIRTMEM_H
#define _MM_VIRTMEM_H

#include <mm/mm_types.h>
#include <nyx/stddef.h>
#include <nyx/types.h>

#include <asi/address.h>
#include <asi/page_data.h>

typedef int vm_sizeclass_t;

#define VM_MAP_OVERMAP (1 << 0)

struct vm_pginfo {
    virt_addr_t    pi_virt_base;
    phys_addr_t    pi_phys_base;
    size_t         pi_len;
    vm_sizeclass_t pi_sc;
    unsigned long  pi_prot;
};

pgd_t *vm_alloc_page_table(int gfp_flags);
void   vm_free_page_table(pgd_t *pgd);
void   vm_free_user_pgtables(pgd_t *pgd);

vm_sizeclass_t vm_sc_for_bytes(size_t bytes);
vm_sizeclass_t vm_lagest_fitting(virt_addr_t va, phys_addr_t pa, size_t len);
vm_sizeclass_t vm_pick_sizeclass(virt_addr_t va, phys_addr_t pa, size_t len, unsigned int flags);
bool           vm_sc_supported(vm_sizeclass_t sc);
size_t         vm_sc_bytes(vm_sizeclass_t sc);
int  vm_map_page(pgd_t *pgd, phys_addr_t pa, virt_addr_t va, unsigned long prot, vm_sizeclass_t sc, int gfp_flags);
int  vm_remap_page(pgd_t *pgd, phys_addr_t pa, virt_addr_t va, unsigned long prot);
int  vm_unmap_page(pgd_t *pgd, virt_addr_t va, vm_sizeclass_t *sc);
int  vm_query_page(pgd_t *pgd, virt_addr_t va, struct vm_pginfo *info);
int  vm_set_page_prot(pgd_t *pgd, virt_addr_t va, unsigned long new_prot, vm_sizeclass_t *sc);
int  vm_split(pgd_t *pgd, virt_addr_t va, int gfp_flags);
int  vm_can_map(virt_addr_t va, phys_addr_t pa, vm_sizeclass_t sc);
int  vm_map(pgd_t *pgd, phys_addr_t pa, virt_addr_t va, size_t len, unsigned long prot, int gfp_flags);
int  vm_remap(pgd_t *pgd, phys_addr_t pa, virt_addr_t va, size_t len, unsigned long prot, int gfp_flags);
int  vm_set_prot(pgd_t *pgd, virt_addr_t va, size_t len, unsigned long new_prot);
int  vm_copy_cow(pgd_t *dst, pgd_t *src, virt_addr_t va, size_t len);
int  vm_unmap(pgd_t *pgd, virt_addr_t va, size_t len);
void vm_release(struct vmspace *vm, virt_addr_t va, size_t len);
int  vm_copy_kernel(pgd_t *dst);
struct page *vm_alloc_page(int gfp_flags);
void         vm_put_page(struct page *pg);
bool         vm_is_addr_user(virt_addr_t va);
void         vm_activate(pgd_t *pgd);

#endif
