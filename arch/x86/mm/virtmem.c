#include <mm/address.h>
#include <mm/mm_types.h>
#include <mm/physmem.h>
#include <mm/virtmem.h>
#include <nyx/errno.h>
#include <nyx/printk.h>
#include <nyx/stddef.h>
#include <nyx/types.h>

#include <asi/bitops.h>
#include <asi/bug.h>
#include <asi/page.h>
#include <asi/page_data.h>
#include <asi/system.h>

#define pr_fmt(fmt) "arch virtmem: " fmt
#ifdef CONFIG_VIRTMEM_DEV_PRINT
#define virtmem_dev_pr(fmt, ...) printk(pr_fmt(fmt), ##__VA_ARGS__)
#else
#define virtmem_dev_pr(fmt, ...)
#endif

#define __PAGE_TABLE_ENTRY_COUNT 512

#define __PAGE_INDEX_MASK 0x1FFull
#define __PAGE_4K_MASK    0xFFFull
#define __PAGE_2M_MASK    0x1FFFFFull
#define __PAGE_1G_MASK    0x3FFFFFFFull
#define __PAGE_ADDR_MASK  0x000FFFFFFFFFF000ull

#define __PAGE_ADDR(pgd) (pgd & __PAGE_ADDR_MASK)

#define __PAGE_4K_SIZE         0x1000ull
#define __PAGE_2M_SIZE         0x200000ull
#define __PAGE_1G_SIZE         0x40000000ull
#define __PAGE_PML4_ENTRY_SIZE 0x8000000000ull

#define __PAGE_PML4_SHIFT 39
#define __PAGE_PDPT_SHIFT 30
#define __PAGE_PDT_SHIFT  21
#define __PAGE_PTT_SHIFT  12

#define __PML4T_IDX(a)    ((a >> __PAGE_PML4_SHIFT) & __PAGE_INDEX_MASK)
#define __PDPT_IDX(a)     ((a >> __PAGE_PDPT_SHIFT) & __PAGE_INDEX_MASK)
#define __PDT_IDX(a)      ((a >> __PAGE_PDT_SHIFT) & __PAGE_INDEX_MASK)
#define __PTT_IDX(a)      ((a >> __PAGE_PTT_SHIFT) & __PAGE_INDEX_MASK)
#define __PG_OFFSET_4K(a) (a & __PAGE_4K_MASK)
#define __PG_OFFSET_2M(a) (a & __PAGE_2M_MASK)
#define __PG_OFFSET_1G(a) (a & __PAGE_1G_MASK)

#define __PG_PRESENT_BIT       0
#define __PG_WRITE_BIT         1
#define __PG_USER_BIT          2
#define __PG_WRITE_THROUGH_BIT 3
#define __PG_CACHE_DISABLE_BIT 4
#define __PG_ACCESSED_BIT      5
#define __PG_NX_BIT            63

#define __PG_LLE_DIRTY_BIT  6
#define __PG_LLE_GLOBAL_BIT 8

#define __PG_PTE_PAT_BIT 7

// flags for 2M and 1G pages (PDE and PDPE)
#define __PG_PDE_PAGE_SIZE_BIT 7
#define __PG_PDE_PAT_BIT       12 // valid only if PS=1

#define __PG_PRESENT       BIT(__PG_PRESENT_BIT)
#define __PG_WRITE         BIT(__PG_WRITE_BIT)
#define __PG_USER          BIT(__PG_USER_BIT)
#define __PG_WRITE_THROUGH BIT(__PG_WRITE_THROUGH_BIT)
#define __PG_CACHE_DISABLE BIT(__PG_CACHE_DISABLE_BIT)
#define __PG_ACCESSED      BIT(__PG_ACCESSED_BIT)
#define __PG_NX            BIT(__PG_NX_BIT)

#define __PG_LLE_DIRTY  BIT(__PG_LLE_DIRTY_BIT)
#define __PG_LLE_GLOBAL BIT(__PG_LLE_GLOBAL_BIT)

#define __PG_PTE_PAT BIT(__PG_PTE_PAT_BIT)

// flags for 2M and 1G pages (PDE and PDPE)
#define __PG_PDE_PAGE_SIZE BIT(__PG_PDE_PAGE_SIZE_BIT)
#define __PG_PDE_PAT       BIT(__PG_PDE_PAT_BIT) // valid only if PS=1

static inline int pgd_index(virt_addr_t virt, int level) {
    switch (level) {
        case 0:
            return __PTT_IDX(virt);
        case 1:
            return __PDT_IDX(virt);
        case 2:
            return __PDPT_IDX(virt);
        case 3:
            return __PML4T_IDX(virt);
    }

    return -1;
}

static inline size_t get_entry_size(int level) {
    switch (level) {
        case 0:
            return __PAGE_4K_SIZE;
        case 1:
            return __PAGE_2M_SIZE;
        case 2:
            return __PAGE_1G_SIZE;
        case 3:
            return __PAGE_PML4_ENTRY_SIZE;
    }

    return 0;
}

/*
 * struct page's private field is used to count the number of
 * active entries in the page table, if PG_pgtable is set.
 */
static inline void inc_pgtable_refcount(pgd_t *pgd) {
    struct page *page = virt_to_page(pgd);
    BUG_ON(!PagePgtable(page));
    page->private++;
}

static inline void dec_pgtable_refcount(pgd_t *pgd) {
    struct page *page = virt_to_page(pgd);
    BUG_ON(!PagePgtable(page));
    page->private--;
}

static inline int is_pgtable_empty(pgd_t *pgd) {
    struct page *page = virt_to_page(pgd);
    BUG_ON(!PagePgtable(page));
    return !page->private;
}

pgd_t *vm_get_page_table(int gfp_flags) {
    phys_addr_t  phys = pm_get_zeroed_page(gfp_flags);
    struct page *page;
    if (phys == INVALID_PHYS_ADDR) { return NULL; }

    page = phys_to_page(phys);
    SetPagePgtable(page);
    page->private = 0;

    return __va(phys);
}

static inline int pte_is_leaf(pgd_t entry, int level) {
    if (!(entry & __PG_PRESENT)) { return 0; }
    if (level == 0) { return 1; }
    return entry & __PG_PDE_PAGE_SIZE;
}

static void __free_page_table(pgd_t *pgd, int level) {
    struct page *page;
    for (int i = 0; i < __PAGE_TABLE_ENTRY_COUNT; i++) {
        if (!pte_is_leaf(pgd[i], level) && (pgd[i] & __PG_PRESENT)) {
            __free_page_table(__va(pgd[i] & __PAGE_ADDR_MASK), level - 1);
        }
    }

    page = virt_to_page(pgd);
    ClearPagePgtable(page);
    pm_free_page((phys_addr_t) __pa(pgd));
}

void vm_free_page_table(pgd_t *pgd) {
    __free_page_table(pgd, 3);
}

static inline u64 get_pte_flags(int flags) {
    u64 pte_flags = 0;

    set_bit(__PG_PRESENT_BIT, &pte_flags);
    if (flags & VM_WRITE) { set_bit(__PG_WRITE_BIT, &pte_flags); }
    if (flags & VM_USER) { set_bit(__PG_USER_BIT, &pte_flags); }
    // TODO: check if CPU supports NX, and if EFER.NXE == 1
    // if (!(flags & VM_EXEC)) { set_bit(__PG_NX_BIT, &pte_flags); }
    if (flags & VM_CACHE_DISABLE) { set_bit(__PG_CACHE_DISABLE_BIT, &pte_flags); }

    return pte_flags;
}

static inline pgd_t *get_table(pgd_t *pgd, int idx, int gfp_flags) {
    pgd_t *out_table;

    if (!(pgd[idx] & __PG_PRESENT)) {
        out_table = vm_get_page_table(gfp_flags);
        if (!out_table) { return NULL; }
        pgd[idx] = (pgd_t) __pa(out_table) | __PG_PRESENT | __PG_WRITE | __PG_USER;
        inc_pgtable_refcount(pgd);
    }

    return (pgd_t *) __va(__PAGE_ADDR(pgd[idx]));
}

static inline int can_map_2m(phys_addr_t phys, virt_addr_t virt, size_t len) {
    if (phys & __PAGE_2M_MASK || virt & __PAGE_2M_MASK) { return 0; }
    if (len < (__PAGE_2M_SIZE >> PAGE_SHIFT)) { return 0; }
    return 1;
}

#ifdef CONFIG_USE_GIGANTIC_PAGES
static inline int can_map_1g(phys_addr_t phys, virt_addr_t virt, size_t len) {
    // TODO: check CPU supports gigantic pages
    if (phys & __PAGE_1G_MASK || virt & __PAGE_1G_MASK) { return 0; }
    if (len < (__PAGE_1G_SIZE >> PAGE_SHIFT)) { return 0; }
    return 1;
}

static inline int map_1g_page(pgd_t *pgd, phys_addr_t phys, virt_addr_t virt, u64 flags, int gfp_flags) {
    pgd_t *pdpt;

    virtmem_dev_pr("mapping 1G page at physical address %#p to virtual %#p with flags 0x%016lx\n", phys, virt, flags);

    pdpt = get_table(pgd, __PML4T_IDX(virt), gfp_flags);
    if (!pdpt) { return -ENOMEM; }

    pdpt[__PDPT_IDX(virt)] = (phys & __PAGE_ADDR_MASK) | __PG_PDE_PAGE_SIZE | flags;
    inc_pgtable_refcount(pdpt);
    invlpg(virt);
    return 0;
}
#endif

static inline int map_2m_page(pgd_t *pgd, phys_addr_t phys, virt_addr_t virt, u64 flags, int gfp_flags) {
    pgd_t *pdpt, *pdt;

    virtmem_dev_pr("mapping 2M page at physical address %#p to virtual %#p with flags 0x%016lx\n", phys, virt, flags);

    pdpt = get_table(pgd, __PML4T_IDX(virt), gfp_flags);
    if (!pdpt) { return -ENOMEM; }

    pdt = get_table(pdpt, __PDPT_IDX(virt), gfp_flags);
    if (!pdt) { return -ENOMEM; }

    pdt[__PDT_IDX(virt)] = (phys & __PAGE_ADDR_MASK) | __PG_PDE_PAGE_SIZE | flags;
    inc_pgtable_refcount(pdt);
    invlpg(virt);
    return 0;
}

static inline int map_4k_page(pgd_t *pgd, phys_addr_t phys, virt_addr_t virt, u64 flags, int gfp_flags) {
    pgd_t *pdpt, *pdt, *ptt;

    virtmem_dev_pr("mapping 4K page at physical address %#p to virtual %#p with flags 0x%016lx\n", phys, virt, flags);

    pdpt = get_table(pgd, __PML4T_IDX(virt), gfp_flags);
    if (!pdpt) { return -ENOMEM; }

    pdt = get_table(pdpt, __PDPT_IDX(virt), gfp_flags);
    if (!pdt) { return -ENOMEM; }

    ptt = get_table(pdt, __PDT_IDX(virt), gfp_flags);
    if (!ptt) { return -ENOMEM; }

    ptt[__PTT_IDX(virt)] = (phys & __PAGE_ADDR_MASK) | flags;
    inc_pgtable_refcount(ptt);
    invlpg(virt);
    return 0;
}

static int vm_arch_map(pgd_t *pgd, phys_addr_t phys, virt_addr_t virt, size_t pg_cnt, u64 flags, int gfp_flags) {
    int res;

    while (pg_cnt) {
#ifdef CONFIG_USE_GIGANTIC_PAGES
        if (can_map_1g(phys, virt, pg_cnt)) {
            if ((res = map_1g_page(pgd, phys, virt, flags, gfp_flags))) { return res; }
            pg_cnt -= __PAGE_1G_SIZE >> PAGE_SHIFT;
            phys += __PAGE_1G_SIZE;
            virt += __PAGE_1G_SIZE;
            continue;
        }
#endif

        if (can_map_2m(phys, virt, pg_cnt)) {
            if ((res = map_2m_page(pgd, phys, virt, flags, gfp_flags))) { return res; }
            pg_cnt -= __PAGE_2M_SIZE >> PAGE_SHIFT;
            phys += __PAGE_2M_SIZE;
            virt += __PAGE_2M_SIZE;
            continue;
        }

        if ((res = map_4k_page(pgd, phys, virt, flags, gfp_flags))) { return res; }
        pg_cnt--;
        phys += __PAGE_4K_SIZE;
        virt += __PAGE_4K_SIZE;
    }

    BUG_ON(pg_cnt != 0);
    return 0;
}

static void vm_arch_umap(pgd_t *pgd, virt_addr_t *virt, size_t *pg_cnt, int level) {
    int idx = pgd_index(*virt, level);

    while (idx < __PAGE_TABLE_ENTRY_COUNT && *pg_cnt) {
        pgd_t *ent        = &pgd[idx];
        size_t entry_size = get_entry_size(level);
        size_t step       = entry_size >> PAGE_SHIFT;
        if (step > *pg_cnt) step = *pg_cnt;

        if (*ent & __PG_PRESENT) {
            if (pte_is_leaf(*ent, level)) {
                virtmem_dev_pr("unmapping level %d entry from address %#p\n", level, virt);
                *ent = 0;
                invlpg(*virt);
                dec_pgtable_refcount(pgd);
            } else {
                pgd_t *child = __va(__PAGE_ADDR(*ent));
                vm_arch_umap(child, virt, pg_cnt, level - 1);

                if (is_pgtable_empty(child)) {
                    __free_page_table(child, level - 1);
                    *ent = 0;
                    dec_pgtable_refcount(pgd);
                }
                idx++;
                continue;
            }
        }

        *virt += step << PAGE_SHIFT;
        *pg_cnt -= step;
        idx++;
    }
}

int vm_map(pgd_t *pgd, phys_addr_t phys, virt_addr_t virt, size_t len, unsigned long flags, int gfp_flags) {
    u64 pte_base_flags;

    if (!pgd || len == 0) { return -EINVAL; }
    if (phys & __PAGE_4K_MASK || virt & __PAGE_4K_MASK || len & __PAGE_4K_MASK) { return -EINVAL; }

    pte_base_flags = get_pte_flags(flags);
    return vm_arch_map(pgd, phys, virt, len >> PAGE_SHIFT, pte_base_flags, gfp_flags);
}

int vm_umap(pgd_t *pgd, virt_addr_t virt, size_t len) {
    if (!pgd) { return -EINVAL; }
    if (virt & __PAGE_4K_MASK || len & __PAGE_4K_SIZE) { return -EINVAL; }

    len >>= PAGE_SHIFT;
    vm_arch_umap(pgd, &virt, &len, 3);

    return 0;
}

void vm_activate(pgd_t *pgd) {
    phys_addr_t pgd_addr = (phys_addr_t) __pa(pgd);

    __asm__ volatile("mov %0, %%cr3"
                     : "=r"(pgd_addr)
                     :
                     : "memory");
}
