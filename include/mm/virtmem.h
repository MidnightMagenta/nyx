#ifndef _MM_VIRTMEM_H
#define _MM_VIRTMEM_H

#include <nyx/stddef.h>
#include <nyx/types.h>

#include <asi/page_data.h>

#define VM_READ          (1 << 0)
#define VM_WRITE         (1 << 1)
#define VM_EXEC          (1 << 2)
#define VM_USER          (1 << 3)
#define VM_CACHE_DISABLE (1 << 4)

pgd_t *vm_get_page_table(int gfp_flags);
void   vm_free_page_table(pgd_t *pgd);

int  vm_map(pgd_t *pgd, phys_addr_t phys, virt_addr_t virt, size_t len, unsigned long flags, int gfp_flags);
int  vm_umap(pgd_t *pgd, virt_addr_t virt, size_t len);
void vm_activate(pgd_t *pgd);

#endif
