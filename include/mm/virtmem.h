#ifndef _MM_VIRTMEM_H
#define _MM_VIRTMEM_H

#include <mm/mm_types.h>
#include <nyx/stddef.h>
#include <nyx/types.h>

#include <asi/page_data.h>

pgd_t *vm_get_page_table(int gfp_flags);
void   vm_free_page_table(pgd_t *pgd);

int  vm_map(pgd_t *pgd, phys_addr_t phys, virt_addr_t virt, size_t len, unsigned long flags, int gfp_flags);
int  vm_umap(pgd_t *pgd, virt_addr_t virt, size_t len);
int  vm_copy(pgd_t *dst, pgd_t *src, int flags);
void vm_activate(pgd_t *pgd);

#endif
