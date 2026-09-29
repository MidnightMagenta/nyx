#include <mm/address.h>
#include <mm/mm_types.h>
#include <mm/physmem.h>
#include <mm/virtmem.h>
#include <nyx/atomic.h>
#include <nyx/errno.h>
#include <nyx/kernel.h>
#include <nyx/linkage.h>
#include <nyx/panic.h>
#include <nyx/refcount.h>
#include <nyx/string.h>
#include <nyx/types.h>

#include <asi/address.h>
#include <asi/bitops.h>
#include <asi/bug.h>
#include <asi/cpufeatures.h>
#include <asi/memory.h>
#include <asi/page.h>
#include <asi/page_data.h>
#include <asi/system.h>

DECLARE_SUBSYS_LOG(virtmem_log);

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

#define __PAGE_4K_PGCNT         1u
#define __PAGE_2M_PGCNT         (__PAGE_2M_SIZE >> PAGE_SHIFT)
#define __PAGE_1G_PGCNT         (__PAGE_1G_SIZE >> PAGE_SHIFT)
#define __PAGE_PML4_ENTRY_PGCNT (__PAGE_PML4_ENTRY_SIZE >> PAGE_SHIFT)

#define __PAGE_PML4_SHIFT 39
#define __PAGE_PDPT_SHIFT 30
#define __PAGE_PDT_SHIFT  21
#define __PAGE_PTT_SHIFT  12

#define __PML4T_IDX(a) ((a >> __PAGE_PML4_SHIFT) & __PAGE_INDEX_MASK)
#define __PDPT_IDX(a)  ((a >> __PAGE_PDPT_SHIFT) & __PAGE_INDEX_MASK)
#define __PDT_IDX(a)   ((a >> __PAGE_PDT_SHIFT) & __PAGE_INDEX_MASK)
#define __PTT_IDX(a)   ((a >> __PAGE_PTT_SHIFT) & __PAGE_INDEX_MASK)

static inline unsigned int pdpt_idx(virt_addr_t va) {
    return __PDPT_IDX(va);
}
static inline unsigned int pdt_idx(virt_addr_t va) {
    return __PDT_IDX(va);
}
static inline unsigned int ptt_idx(virt_addr_t va) {
    return __PTT_IDX(va);
}

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

#define VM_SC_4K 0
#define VM_SC_2M 1
#define VM_SC_1G 2

#ifdef CONFIG_USE_GIGANTIC_PAGES
#define VM_SC_CLASS_COUNT 3
#else
#define VM_SC_CLASS_COUNT 2
#endif

phys_addr_t kernel_pdpts[256];

struct {
    bool   supported;
    size_t bytes;
    int    max_ptes_none;
} vm_sc_info[VM_SC_CLASS_COUNT] = {
        {true, __PAGE_4K_SIZE, 0},
        {true, __PAGE_2M_SIZE, 100},
};

/*
 * struct page's refcnt_private field is used to count the number of
 * active entries in the page table, if PG_pgtable is set.
 */
static inline void set_pgtable_refcount(pgd_t *pgd, int rc) {
    struct page *page = virt_to_page(pgd);
    BUG_ON(!PagePgtable(page));
    refcount_set(&page->pg_refcnt_private, rc);
}

static inline int inc_pgtable_refcount(pgd_t *pgd) {
    struct page *page = virt_to_page(pgd);
    BUG_ON(!PagePgtable(page));
    return refcount_get_inc(&page->pg_refcnt_private);
}

static inline int dec_pgtable_refcount(pgd_t *pgd) {
    struct page *page = virt_to_page(pgd);
    BUG_ON(!PagePgtable(page));
    return refcount_get_dec(&page->pg_refcnt_private);
}

static inline int is_pgtable_empty(pgd_t *pgd) {
    struct page *page = virt_to_page(pgd);
    BUG_ON(!PagePgtable(page));
    return !refcount_get(&page->pg_refcnt_private);
}

void __init virtmem_init() {
    // 2M pages
    vm_sc_info[VM_SC_2M].bytes         = __PAGE_2M_SIZE;
    vm_sc_info[VM_SC_2M].supported     = true;
    vm_sc_info[VM_SC_2M].max_ptes_none = 100;
}

#define KERNEL_PML4_FIRST_IDX 256

void __init prepopulate_pdpt_entries(pgd_t *pgd) {
    pgd_t *pdpt;

    for (unsigned int idx = KERNEL_PML4_FIRST_IDX; idx < __PAGE_TABLE_ENTRY_COUNT; idx++) {
        if (pgd[idx] & __PG_PRESENT) {
            pdpt = __va(__PAGE_ADDR(pgd[idx]));
            BUG_ON(!PagePgtable(virt_to_page(pdpt)));
        } else {
            pdpt = vm_alloc_page_table(M_SLEEPOK);
            if (!pdpt) { BUG(); }
            pgd[idx] = (pgd_t) __pa(pdpt) | __PG_PRESENT | __PG_WRITE;
            inc_pgtable_refcount(pgd);
        }

        kernel_pdpts[idx - KERNEL_PML4_FIRST_IDX] = pgd[idx];
        inc_pgtable_refcount(pdpt); /* permanent kernel pin - never freed */
    }
}

vm_sizeclass_t vm_sc_for_bytes(size_t bytes) {
    for (int sc = 0; sc < VM_SC_CLASS_COUNT; sc++) {
        if (!vm_sc_supported(sc)) { continue; }

        if (bytes > vm_sc_info[sc].bytes) { continue; }
        return (vm_sizeclass_t) sc;
    }

    return -1;
}

vm_sizeclass_t vm_lagest_fitting(virt_addr_t va, phys_addr_t pa, size_t len) {
    for (int sc = VM_SC_CLASS_COUNT - 1; sc >= VM_SC_4K; sc--) {
        size_t bytes;

        if (!vm_sc_supported(sc)) { continue; }

        bytes = vm_sc_info[sc].bytes;
        if (len < bytes) { continue; }
        if ((pa & (bytes - 1)) || (va & (bytes - 1))) { continue; }

        return (vm_sizeclass_t) sc;
    }

    return VM_SC_4K;
}

vm_sizeclass_t vm_pick_sizeclass(virt_addr_t va, phys_addr_t pa, size_t len, unsigned int flags) {
    for (int sc = VM_SC_CLASS_COUNT - 1; sc >= VM_SC_4K; sc--) {
        size_t bytes, max_waste;

        if (!vm_sc_supported(sc)) { continue; }

        bytes = vm_sc_info[sc].bytes;
        if ((pa & (bytes - 1)) || (va & (bytes - 1))) { continue; }
        if (len >= bytes) { return (vm_sizeclass_t) sc; }

        if (sc == VM_SC_4K) { return VM_SC_4K; }
        if (!(flags & VM_MAP_OVERMAP)) { continue; }
        max_waste = (bytes >> ilog2((unsigned int) __PAGE_TABLE_ENTRY_COUNT)) * vm_sc_info[sc].max_ptes_none;
        if ((bytes - len) > max_waste) { continue; }

        return (vm_sizeclass_t) sc;
    }

    return VM_SC_4K;
}

bool vm_sc_supported(vm_sizeclass_t sc) {
    return vm_sc_info[sc].supported;
}

size_t vm_sc_bytes(vm_sizeclass_t sc) {
    return vm_sc_info[sc].bytes;
}

pgd_t *vm_alloc_page_table(int gfp_flags) {
    phys_addr_t  phys = pm_get_zeroed_page(gfp_flags);
    struct page *page;
    if (phys == INVALID_PHYS_ADDR) { return NULL; }

    page = phys_to_page(phys);
    SetPagePgtable(page);
    refcount_init(&page->pg_refcnt_private, 0);

    return __va(phys);
}

void vm_free_page_table(pgd_t *pgd) {
    struct page *page;

    BUG_ON(!is_pgtable_empty(pgd));

    page = virt_to_page(pgd);
    ClearPagePgtable(page);
    __pm_free_page(page);
}

static inline u64 get_pte_flags(unsigned long flags) {
    u64 pte_flags = 0;

    set_bit(__PG_PRESENT_BIT, &pte_flags);
    if (flags & VM_WRITE) { set_bit(__PG_WRITE_BIT, &pte_flags); }
    if (flags & VM_USER) { set_bit(__PG_USER_BIT, &pte_flags); }
    // TODO: minor, VMM/EXEC_PROT - check if CPU supports NX, and if EFER.NXE == 1
    // if (!(flags & VM_EXEC)) { set_bit(__PG_NX_BIT, &pte_flags); }
    if (flags & VM_CACHE_DISABLE) { set_bit(__PG_CACHE_DISABLE_BIT, &pte_flags); }

    return pte_flags;
}

static inline pgd_t *get_pgtable(pgd_t *pgd, size_t idx, bool allocate, int gfp_flags) {
    pgd_t *out_table = NULL;

    if (pgd[idx] & __PG_PRESENT) {
        out_table = __va(__PAGE_ADDR(pgd[idx]));
    } else if (allocate && !(pgd[idx] & __PG_PRESENT)) {
        out_table = vm_alloc_page_table(gfp_flags);
        if (!out_table) { return NULL; }
        pgd[idx] = (pgd_t) __pa(out_table) | __PG_PRESENT | __PG_WRITE | __PG_USER;
        inc_pgtable_refcount(pgd);
    }

    return out_table;
}

static const struct {
    vm_sizeclass_t terminates_at;
    unsigned int   (*index_of)(virt_addr_t va);
} pt_levels[] = {
        {VM_SC_1G, pdpt_idx},
        {VM_SC_2M, pdt_idx},
        {VM_SC_4K, ptt_idx},
};

#define MAX_PT_DEPTH (ARRAY_SIZE(pt_levels) + 1)

struct pt_walk_entry {
    pgd_t       *table;
    unsigned int idx;
};

static inline int walk(pgd_t *pgd, virt_addr_t va, struct pt_walk_entry *path, int *depth, vm_sizeclass_t *sc) {
    pgd_t       *table = pgd;
    unsigned int idx   = __PML4T_IDX(va);
    int          d     = 0;

    *depth = 0;

    path[d++] = (struct pt_walk_entry){table, idx};
    *depth    = d;
    if (!(table[idx] & __PG_PRESENT)) { return -ENOENT; }
    table = __va(__PAGE_ADDR(table[idx]));

    for (size_t i = 0; i < ARRAY_SIZE(pt_levels); i++) {
        idx       = pt_levels[i].index_of(va);
        path[d++] = (struct pt_walk_entry){table, idx};
        *depth    = d;

        if (!(table[idx] & __PG_PRESENT)) { return -ENOENT; }

        if ((pt_levels[i].terminates_at == VM_SC_4K) || (table[idx] & __PG_PDE_PAGE_SIZE)) {
            *sc = pt_levels[i].terminates_at;
            return 0;
        }

        table = __va(__PAGE_ADDR(table[idx]));
    }

    BUG();
    __unreachable;
}

static void prune_empty_tables(struct pt_walk_entry *path, int depth) {
    for (int i = depth - 1; i > 0; i--) {
        pgd_t *pt = path[i].table;

        if (!is_pgtable_empty(pt)) { break; }

        pgd_t       *parent     = path[i - 1].table;
        unsigned int parent_idx = path[i - 1].idx;

        vm_free_page_table(pt);
        parent[parent_idx] = 0;
        dec_pgtable_refcount(parent);
    }
}

int vm_map_page(pgd_t *pgd, phys_addr_t pa, virt_addr_t va, unsigned long prot, vm_sizeclass_t sc, int gfp_flags) {
    pgd_t               *table;
    u64                  entry_flags;
    struct pt_walk_entry path[MAX_PT_DEPTH];
    int                  depth = 0;
    unsigned int         idx;

    pr_debug(virtmem_log,
             "Mapping %#lx to va %#lx with flags %#lx for %ld bytes from pgd %#p\n",
             pa,
             va,
             prot,
             vm_sc_bytes(sc),
             pgd);

    if (sc >= VM_SC_CLASS_COUNT) {
        BUG();
        return -EINVAL;
    }
    if (!vm_sc_info[sc].supported) {
        BUG();
        return -EINVAL;
    }
    if ((pa & (vm_sc_info[sc].bytes - 1)) || (va & (vm_sc_info[sc].bytes - 1))) {
        BUG();
        return -EINVAL;
    }

    path[depth++] = (struct pt_walk_entry){pgd, __PML4T_IDX(va)};
    table         = get_pgtable(pgd, __PML4T_IDX(va), true, gfp_flags);
    if (!table) { return -ENOMEM; }

    for (size_t i = 0; i < ARRAY_SIZE(pt_levels); i++) {
        idx           = pt_levels[i].index_of(va);
        path[depth++] = (struct pt_walk_entry){table, idx};

        if (sc == pt_levels[i].terminates_at) {
            if (table[idx] & __PG_PRESENT) { return -EEXIST; }

            entry_flags = get_pte_flags(prot);
            if (sc != VM_SC_4K) { entry_flags |= __PG_PDE_PAGE_SIZE; }
            table[idx] = (pa & __PAGE_ADDR_MASK) | entry_flags;
            inc_pgtable_refcount(table);
            invlpg(va);
            return 0;
        }

        if ((table[idx] & __PG_PRESENT) && (table[idx] & __PG_PDE_PAGE_SIZE)) { return -EEXIST; }

        table = get_pgtable(table, idx, true, gfp_flags);
        if (!table) {
            prune_empty_tables(path, depth);
            return -ENOMEM;
        }
    }

    BUG();
    return -EINVAL;
}

int vm_remap_page(pgd_t *pgd, phys_addr_t pa, virt_addr_t va, unsigned long prot) {
    struct pt_walk_entry path[MAX_PT_DEPTH];
    vm_sizeclass_t       sc;
    int                  depth;
    int                  res;

    pr_debug(virtmem_log, "Remapping %#lx to pa %#lx from pgd %#p\n", va, pa, pgd);

    if (va & __PAGE_4K_MASK) {
        BUG();
        return -EINVAL;
    }

    if ((res = walk(pgd, va, path, &depth, &sc))) { return res; }

    struct pt_walk_entry leaf = path[depth - 1];

    pr_debug(virtmem_log, "    found entry at index %d in table at depth %d at %#p\n", leaf.idx, depth, leaf.table);
    pr_debug(virtmem_log, "    value of entry: %#lx\n", leaf.table[leaf.idx]);

    leaf.table[leaf.idx] = (pa & __PAGE_ADDR_MASK) | get_pte_flags(prot);
    inc_pgtable_refcount(leaf.table);
    invlpg(va);

    return 0;
}

int vm_unmap_page(pgd_t *pgd, virt_addr_t va, vm_sizeclass_t *sc) {
    struct pt_walk_entry path[MAX_PT_DEPTH];
    int                  depth;
    int                  res;

    pr_debug(virtmem_log, "Unmapping %#lx from pgd %#p\n", va, pgd);

    if (va & __PAGE_4K_MASK) {
        BUG();
        return -EINVAL;
    }

    if ((res = walk(pgd, va, path, &depth, sc))) { return res; }

    struct pt_walk_entry leaf = path[depth - 1];

    pr_debug(virtmem_log, "    found entry at index %d in table at depth %d at %#p\n", leaf.idx, depth, leaf.table);
    pr_debug(virtmem_log, "    value of entry: %#lx\n", leaf.table[leaf.idx]);

    leaf.table[leaf.idx] = 0;
    invlpg(va);
    dec_pgtable_refcount(leaf.table);
    prune_empty_tables(path, depth);

    return 0;
}

static unsigned long pg_flags_to_vm_flags(u64 pg_flags) {
    unsigned long flags = 0;

    if (pg_flags & __PG_WRITE) { flags |= VM_WRITE; }
    if (pg_flags & __PG_USER) { flags |= VM_USER; }
    if (!(pg_flags & __PG_NX)) { flags |= VM_EXEC; }
    if (pg_flags & __PG_CACHE_DISABLE) { flags |= VM_CACHE_DISABLE; }

    return flags;
}

int vm_query_page(pgd_t *pgd, virt_addr_t va, struct vm_pginfo *info) {
    struct pt_walk_entry path[MAX_PT_DEPTH];
    int                  depth;
    int                  res;
    pgd_t               *table;
    unsigned int         idx;

    if (va & __PAGE_4K_MASK) {
        BUG();
        return -EINVAL;
    }

    res = walk(pgd, va, path, &depth, &info->pi_sc);
    if (res == -ENOENT) {
        switch (depth) {
            case 1:
                info->pi_len = __PAGE_PML4_ENTRY_SIZE;
                break;
            case 2:
                info->pi_len = __PAGE_1G_SIZE;
                break;
            case 3:
                info->pi_len = __PAGE_2M_SIZE;
                break;
            default:
                info->pi_len = __PAGE_4K_SIZE;
                break;
        }
        info->pi_virt_base = va & ~(info->pi_len - 1);
        info->pi_sc        = -1;
        return -ENOENT;
    } else if (res) {
        return res;
    }

    table = path[depth - 1].table;
    idx   = path[depth - 1].idx;

    info->pi_len       = vm_sc_bytes(info->pi_sc);
    info->pi_virt_base = va & ~(info->pi_len - 1);
    info->pi_phys_base = __PAGE_ADDR(table[idx]);
    info->pi_prot      = pg_flags_to_vm_flags(table[idx] & ~__PAGE_ADDR_MASK);

    return 0;
}

int vm_set_page_prot(pgd_t *pgd, virt_addr_t va, unsigned long new_prot, vm_sizeclass_t *sc) {
    struct pt_walk_entry path[MAX_PT_DEPTH];
    int                  depth;
    pgd_t               *table;
    unsigned int         idx;
    phys_addr_t          old_pa;
    u64                  pg_flags;
    int                  res;

    pr_debug(virtmem_log, "Changing prot for %#lx - new prot: %#lx for pgd %#p\n", va, new_prot, pgd);

    if (va & __PAGE_4K_MASK) {
        BUG();
        return -EINVAL;
    }

    if ((res = walk(pgd, va, path, &depth, sc))) { return res; }

    table = path[depth - 1].table;
    idx   = path[depth - 1].idx;

    pr_debug(virtmem_log, "    old entry %#lx", table[idx]);

    if (va & (vm_sc_bytes(*sc) - 1)) { return -EINVAL; }

    old_pa     = __PAGE_ADDR(table[idx]);
    pg_flags   = get_pte_flags(new_prot) | (sc != VM_SC_4K ? __PG_PDE_PAGE_SIZE : 0);
    table[idx] = old_pa | pg_flags;
    invlpg(va);

    return 0;
    return 0;
}

int vm_split(pgd_t *pgd, virt_addr_t va, int gfp_flags) {
    struct pt_walk_entry path[MAX_PT_DEPTH];
    int                  depth, res;
    vm_sizeclass_t       sc, child_sc;
    struct pt_walk_entry leaf;
    u64                  orig, child_flags;
    phys_addr_t          base_pa;
    size_t               child_bytes;
    pgd_t               *new_pt;

    pr_debug(virtmem_log, "Splitting page at %#lx for pgd %#p\n", va, pgd);

    if ((res = walk(pgd, va, path, &depth, &sc))) { return res; }
    if (sc == VM_SC_4K) { return 0; }

    leaf        = path[depth - 1];
    orig        = leaf.table[leaf.idx];
    child_sc    = sc - 1;
    child_bytes = vm_sc_bytes(child_sc);
    base_pa     = __PAGE_ADDR(orig);

    pr_debug(virtmem_log, "    old pa was %#lx for %ld bytes. Splitting to %ld bytes\n", orig, sc, child_sc);

    child_flags = orig & ~(__PAGE_ADDR_MASK | __PG_PDE_PAGE_SIZE);
    if (child_sc != VM_SC_4K) { child_flags |= __PG_PDE_PAGE_SIZE; }

    new_pt = vm_alloc_page_table(gfp_flags);
    if (!new_pt) { return -ENOMEM; }

    for (int n = 0; n < __PAGE_TABLE_ENTRY_COUNT; n++) {
        new_pt[n] = ((base_pa + (phys_addr_t) n * child_bytes) & __PAGE_ADDR_MASK) | child_flags;
    }
    set_pgtable_refcount(new_pt, __PAGE_TABLE_ENTRY_COUNT);

    barrier();

    leaf.table[leaf.idx] = (pgd_t) __pa(new_pt) | __PG_PRESENT | __PG_WRITE | __PG_USER;
    invlpg(va); /* SMP: needs a shootdown */

    return 0;
}

int vm_can_map(virt_addr_t va, phys_addr_t pa, vm_sizeclass_t sc) {
    size_t bytes;

    if (sc >= VM_SC_CLASS_COUNT) { BUG(); }
    bytes = vm_sc_bytes(sc);
    return !((va & (bytes - 1)) || (pa & (bytes - 1)));
}

int vm_copy_kernel(pgd_t *dst) {
    for (int i = 0; i < 256; i++) { dst[KERNEL_PML4_FIRST_IDX + i] = kernel_pdpts[i]; }
    return 0;
}

bool vm_is_addr_user(virt_addr_t va) {
    return va <= ARCH_USER_END;
}

void vm_activate(pgd_t *pgd) {
    phys_addr_t pgd_addr = (phys_addr_t) __pa(pgd);

    __asm__ volatile("mov %0, %%cr3"
                     :
                     : "r"(pgd_addr)
                     : "memory");
}
