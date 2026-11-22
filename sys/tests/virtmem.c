#include <sys/errno.h>
#include <sys/mm/address.h>
#include <sys/mm/physmem.h>
#include <sys/mm/virtmem.h>
#include <sys/string.h>
#include <sys/testing.h>

#include <asi/memory.h>
#include <asi/page.h>

#define SZ_4K 0x1000ul
#define SZ_2M 0x200000ul
#define SZ_1G 0x40000000ul

#define TEST_VA ((virt_addr_t) 0x40000000)

#define PGD_ENTRIES (PAGE_SIZE / sizeof(pgd_t))

static pgd_t *new_pgd() {
    return vm_alloc_page_table(0);
}

static bool pgd_is_empty(pgd_t *pgd) {
    for (size_t i = 0; i < PGD_ENTRIES; i++) {
        if (pgd[i]) { return false; }
    }
    return true;
}

static void free_pgd(struct test_state *state, pgd_t *pgd) {
    if (!pgd_is_empty(pgd)) {
        ADD_FAILURE("page tables were not pruned after unmapping everything, leaking pgd %p", pgd);
        return;
    }
    vm_free_page_table(pgd);
}

KERNEL_TEST(virtmem_sizeclass) {
    EXPECT_EQ(vm_sc_bytes(0), SZ_4K);
    EXPECT_EQ(vm_sc_bytes(1), SZ_2M);
    EXPECT_TRUE(vm_sc_supported(0));
    EXPECT_TRUE(vm_sc_supported(1));

    EXPECT_EQ(vm_sc_for_bytes(1), 0);
    EXPECT_EQ(vm_sc_for_bytes(SZ_4K), 0);
    EXPECT_EQ(vm_sc_for_bytes(SZ_4K + 1), 1);
    EXPECT_EQ(vm_sc_for_bytes(SZ_2M), 1);

    EXPECT_EQ(vm_pick_sizeclass(SZ_2M, SZ_2M, SZ_2M, 0), 1);
    EXPECT_EQ(vm_pick_sizeclass(SZ_2M, SZ_2M, 4 * SZ_2M, 0), 1);
    EXPECT_EQ(vm_pick_sizeclass(SZ_2M + SZ_4K, SZ_2M, SZ_2M, 0), 0);
    EXPECT_EQ(vm_pick_sizeclass(SZ_2M, SZ_2M + SZ_4K, SZ_2M, 0), 0);
    EXPECT_EQ(vm_pick_sizeclass(SZ_2M, SZ_2M, SZ_2M - SZ_4K, 0), 0);
    EXPECT_EQ(vm_pick_sizeclass(SZ_2M, SZ_2M, SZ_2M - SZ_4K, VM_MAP_OVERMAP), 1);
    EXPECT_EQ(vm_pick_sizeclass(SZ_2M, SZ_2M, SZ_4K, VM_MAP_OVERMAP), 0);

    EXPECT_EQ(vm_lagest_fitting(SZ_2M, SZ_2M, SZ_2M), 1);
    EXPECT_EQ(vm_lagest_fitting(SZ_2M, SZ_2M, SZ_2M - 1), 0);
    EXPECT_EQ(vm_lagest_fitting(SZ_4K, SZ_2M, SZ_2M), 0);

    EXPECT_TRUE(vm_can_map(SZ_2M, 3 * SZ_2M, 1));
    EXPECT_FALSE(vm_can_map(SZ_2M + SZ_4K, 3 * SZ_2M, 1));
    EXPECT_TRUE(vm_can_map(SZ_4K, 5 * SZ_4K, 0));

    EXPECT_TRUE(vm_is_addr_user(ARCH_USER_START));
    EXPECT_TRUE(vm_is_addr_user(ARCH_USER_END));
    EXPECT_FALSE(vm_is_addr_user(ARCH_KERNEL_START));
    EXPECT_FALSE(vm_is_addr_user(ARCH_DIRECT_MAP_BASE));
}

KERNEL_TEST(virtmem_page_table_alloc) {
    pgd_t *pgd = new_pgd();

    ASSERT_NOT_NULL(pgd);
    EXPECT_EQ((uintptr_t) pgd & (PAGE_SIZE - 1), 0);
    EXPECT_TRUE(PagePgtable(virt_to_page(pgd)));
    EXPECT_TRUE(pgd_is_empty(pgd));

    vm_free_page_table(pgd);
}

KERNEL_TEST(virtmem_map_4k) {
    struct vm_pginfo info;
    vm_sizeclass_t   sc = -1;
    pgd_t           *pgd;
    phys_addr_t      pa = 0x123000;

    pgd = new_pgd();
    ASSERT_NOT_NULL(pgd);

    EXPECT_EQ(vm_query_page(pgd, TEST_VA, &info), -ENOENT);
    EXPECT_EQ(info.pi_len, 512 * SZ_1G);

    ASSERT_OK(vm_map_page(pgd, pa, TEST_VA, VM_READ | VM_WRITE | VM_USER, 0, 0));
    EXPECT_FALSE(pgd_is_empty(pgd));

    EXPECT_OK(vm_query_page(pgd, TEST_VA, &info));
    EXPECT_EQ(info.pi_sc, 0);
    EXPECT_EQ(info.pi_len, SZ_4K);
    EXPECT_EQ(info.pi_virt_base, TEST_VA);
    EXPECT_EQ(info.pi_phys_base, pa);
    EXPECT_TRUE(info.pi_prot & VM_WRITE);
    EXPECT_TRUE(info.pi_prot & VM_USER);
    EXPECT_FALSE(info.pi_prot & VM_CACHE_DISABLE);

    EXPECT_EQ(vm_query_page(pgd, TEST_VA + SZ_4K, &info), -ENOENT);
    EXPECT_EQ(info.pi_len, SZ_4K);
    EXPECT_EQ(info.pi_sc, -1);

    EXPECT_EQ(vm_map_page(pgd, pa, TEST_VA, VM_READ, 0, 0), -EEXIST);

    EXPECT_OK(vm_unmap_page(pgd, TEST_VA, &sc));
    EXPECT_EQ(sc, 0);
    EXPECT_EQ(vm_query_page(pgd, TEST_VA, &info), -ENOENT);
    EXPECT_EQ(vm_unmap_page(pgd, TEST_VA, &sc), -ENOENT);

    free_pgd(state, pgd);
}

KERNEL_TEST(virtmem_map_prot) {
    struct vm_pginfo info;
    vm_sizeclass_t   sc;
    pgd_t           *pgd = new_pgd();

    ASSERT_NOT_NULL(pgd);

    EXPECT_OK(vm_map_page(pgd, 0x1000, TEST_VA, VM_READ, 0, 0));
    EXPECT_OK(vm_map_page(pgd, 0x2000, TEST_VA + SZ_4K, VM_READ | VM_CACHE_DISABLE, 0, 0));

    EXPECT_OK(vm_query_page(pgd, TEST_VA, &info));
    EXPECT_FALSE(info.pi_prot & VM_WRITE);
    EXPECT_FALSE(info.pi_prot & VM_USER);
    EXPECT_OK(vm_query_page(pgd, TEST_VA + SZ_4K, &info));
    EXPECT_TRUE(info.pi_prot & VM_CACHE_DISABLE);

    EXPECT_OK(vm_set_page_prot(pgd, TEST_VA, VM_READ | VM_WRITE | VM_USER, &sc));
    EXPECT_EQ(sc, 0);
    EXPECT_OK(vm_query_page(pgd, TEST_VA, &info));
    EXPECT_TRUE(info.pi_prot & VM_WRITE);
    EXPECT_TRUE(info.pi_prot & VM_USER);
    EXPECT_EQ(info.pi_phys_base, 0x1000);
    EXPECT_EQ(info.pi_sc, 0);

    EXPECT_OK(vm_set_prot(pgd, TEST_VA, 2 * SZ_4K, VM_READ));
    EXPECT_OK(vm_query_page(pgd, TEST_VA, &info));
    EXPECT_FALSE(info.pi_prot & VM_WRITE);
    EXPECT_OK(vm_query_page(pgd, TEST_VA + SZ_4K, &info));
    EXPECT_FALSE(info.pi_prot & VM_CACHE_DISABLE);
    EXPECT_EQ(info.pi_phys_base, 0x2000);

    EXPECT_EQ(vm_set_page_prot(pgd, TEST_VA + 2 * SZ_4K, VM_READ, &sc), -ENOENT);

    EXPECT_OK(vm_unmap(pgd, TEST_VA, 2 * SZ_4K));
    free_pgd(state, pgd);
}

KERNEL_TEST(virtmem_map_2m) {
    struct vm_pginfo info;
    pgd_t           *pgd = new_pgd();
    phys_addr_t      pa  = 0x40000000;

    ASSERT_NOT_NULL(pgd);

    ASSERT_OK(vm_map(pgd, pa, TEST_VA, SZ_2M, VM_READ | VM_WRITE, 0));

    EXPECT_OK(vm_query_page(pgd, TEST_VA + 5 * SZ_4K, &info));
    EXPECT_EQ(info.pi_sc, 1);
    EXPECT_EQ(info.pi_len, SZ_2M);
    EXPECT_EQ(info.pi_virt_base, TEST_VA);
    EXPECT_EQ(info.pi_phys_base, pa);
    EXPECT_TRUE(info.pi_prot & VM_WRITE);

    EXPECT_EQ(vm_map_page(pgd, 0x1000, TEST_VA + SZ_4K, VM_READ, 0, 0), -EEXIST);

    vm_sizeclass_t sc;
    EXPECT_OK(vm_set_page_prot(pgd, TEST_VA, VM_READ, &sc));
    EXPECT_EQ(sc, 1);
    EXPECT_OK(vm_query_page(pgd, TEST_VA, &info));
    EXPECT_EQ(info.pi_sc, 1);
    EXPECT_FALSE(info.pi_prot & VM_WRITE);

    EXPECT_OK(vm_unmap(pgd, TEST_VA, SZ_2M));
    EXPECT_EQ(vm_query_page(pgd, TEST_VA, &info), -ENOENT);
    free_pgd(state, pgd);
}

KERNEL_TEST(virtmem_map_mixed_range) {
    struct vm_pginfo info;
    pgd_t           *pgd = new_pgd();
    virt_addr_t      va  = TEST_VA + SZ_2M - 2 * SZ_4K;
    phys_addr_t      pa  = 0x80000000 + SZ_2M - 2 * SZ_4K;
    size_t           len = SZ_2M + 4 * SZ_4K;

    ASSERT_NOT_NULL(pgd);
    ASSERT_OK(vm_map(pgd, pa, va, len, VM_READ, 0));

    EXPECT_OK(vm_query_page(pgd, va, &info));
    EXPECT_EQ(info.pi_sc, 0);
    EXPECT_EQ(info.pi_phys_base, pa);
    EXPECT_OK(vm_query_page(pgd, va + SZ_4K, &info));
    EXPECT_EQ(info.pi_sc, 0);
    EXPECT_OK(vm_query_page(pgd, TEST_VA + SZ_2M, &info));
    EXPECT_EQ(info.pi_sc, 1);
    EXPECT_EQ(info.pi_phys_base, 0x80000000 + SZ_2M);
    EXPECT_OK(vm_query_page(pgd, TEST_VA + 2 * SZ_2M, &info));
    EXPECT_EQ(info.pi_sc, 0);
    EXPECT_OK(vm_query_page(pgd, TEST_VA + 2 * SZ_2M + SZ_4K, &info));
    EXPECT_EQ(info.pi_phys_base, 0x80000000 + 2 * SZ_2M + SZ_4K);
    EXPECT_EQ(vm_query_page(pgd, TEST_VA + 2 * SZ_2M + 2 * SZ_4K, &info), -ENOENT);
    EXPECT_EQ(vm_query_page(pgd, va - SZ_4K, &info), -ENOENT);

    EXPECT_OK(vm_unmap(pgd, va, len));
    free_pgd(state, pgd);
}

KERNEL_TEST(virtmem_map_overlap_rollback) {
    struct vm_pginfo info;
    pgd_t           *pgd = new_pgd();

    ASSERT_NOT_NULL(pgd);

    ASSERT_OK(vm_map_page(pgd, 0x5000, TEST_VA + 3 * SZ_4K, VM_READ, 0, 0));

    EXPECT_EQ(vm_map(pgd, 0x10000, TEST_VA, 8 * SZ_4K, VM_READ, 0), -EEXIST);
    EXPECT_EQ(vm_query_page(pgd, TEST_VA, &info), -ENOENT);
    EXPECT_EQ(vm_query_page(pgd, TEST_VA + 2 * SZ_4K, &info), -ENOENT);
    EXPECT_EQ(vm_query_page(pgd, TEST_VA + 4 * SZ_4K, &info), -ENOENT);
    EXPECT_OK(vm_query_page(pgd, TEST_VA + 3 * SZ_4K, &info));
    EXPECT_EQ(info.pi_phys_base, 0x5000);

    EXPECT_OK(vm_unmap(pgd, TEST_VA, 8 * SZ_4K));
    free_pgd(state, pgd);
}

KERNEL_TEST(virtmem_unmap_sparse_range) {
    struct vm_pginfo info;
    pgd_t           *pgd    = new_pgd();
    virt_addr_t      target = TEST_VA + SZ_2M;

    ASSERT_NOT_NULL(pgd);
    ASSERT_OK(vm_map_page(pgd, 0x7000, target, VM_READ, 0, 0));

    EXPECT_OK(vm_unmap(pgd, TEST_VA + SZ_4K, SZ_2M + SZ_4K));
    EXPECT_EQ(vm_query_page(pgd, target, &info), -ENOENT);

    if (vm_query_page(pgd, target, &info) == 0) { vm_unmap_page(pgd, target, &(vm_sizeclass_t){0}); }
    free_pgd(state, pgd);
}

KERNEL_TEST(virtmem_remap) {
    struct vm_pginfo info;
    vm_sizeclass_t   sc;
    pgd_t           *pgd = new_pgd();

    ASSERT_NOT_NULL(pgd);
    ASSERT_OK(vm_map_page(pgd, 0x1000, TEST_VA, VM_READ, 0, 0));

    EXPECT_OK(vm_remap_page(pgd, 0x9000, TEST_VA, VM_READ | VM_WRITE));
    EXPECT_OK(vm_query_page(pgd, TEST_VA, &info));
    EXPECT_EQ(info.pi_phys_base, 0x9000);
    EXPECT_TRUE(info.pi_prot & VM_WRITE);

    EXPECT_EQ(vm_remap_page(pgd, 0x9000, TEST_VA + SZ_4K, VM_READ), -ENOENT);

    EXPECT_OK(vm_unmap_page(pgd, TEST_VA, &sc));
    free_pgd(state, pgd);
}

KERNEL_TEST(virtmem_split) {
    struct vm_pginfo info;
    pgd_t           *pgd = new_pgd();
    phys_addr_t      pa  = 0x40000000;

    ASSERT_NOT_NULL(pgd);
    ASSERT_OK(vm_map(pgd, pa, TEST_VA, SZ_2M, VM_READ | VM_USER, 0));

    EXPECT_OK(vm_split(pgd, TEST_VA + 7 * SZ_4K, 0));

    EXPECT_OK(vm_query_page(pgd, TEST_VA + 7 * SZ_4K, &info));
    EXPECT_EQ(info.pi_sc, 0);
    EXPECT_EQ(info.pi_virt_base, TEST_VA + 7 * SZ_4K);
    EXPECT_EQ(info.pi_phys_base, pa + 7 * SZ_4K);
    EXPECT_TRUE(info.pi_prot & VM_USER);
    EXPECT_FALSE(info.pi_prot & VM_WRITE);

    EXPECT_OK(vm_query_page(pgd, TEST_VA + SZ_2M - SZ_4K, &info));
    EXPECT_EQ(info.pi_phys_base, pa + SZ_2M - SZ_4K);

    EXPECT_OK(vm_split(pgd, TEST_VA, 0));

    EXPECT_OK(vm_unmap(pgd, TEST_VA, SZ_4K));
    EXPECT_EQ(vm_query_page(pgd, TEST_VA, &info), -ENOENT);
    EXPECT_OK(vm_query_page(pgd, TEST_VA + SZ_4K, &info));

    EXPECT_OK(vm_unmap(pgd, TEST_VA, SZ_2M));
    free_pgd(state, pgd);
}

KERNEL_TEST(virtmem_copy_cow_release) {
    struct vm_pginfo info;
    struct vmspace   src_vs, dst_vs;
    struct page     *pg;
    pgd_t           *src = new_pgd();
    pgd_t           *dst = new_pgd();

    ASSERT_NOT_NULL(src);
    ASSERT_NOT_NULL(dst);

    pg = vm_alloc_page(0);
    ASSERT_NOT_NULL(pg);
    EXPECT_EQ(refcount_get(&pg->pg_refcnt), 1);

    ASSERT_OK(vm_map_page(src, page_to_phys(pg), TEST_VA, VM_READ | VM_WRITE | VM_USER, 0, 0));
    ASSERT_OK(vm_copy_cow(dst, src, TEST_VA, SZ_4K));

    EXPECT_EQ(refcount_get(&pg->pg_refcnt), 2);
    EXPECT_OK(vm_query_page(src, TEST_VA, &info));
    EXPECT_FALSE(info.pi_prot & VM_WRITE);
    EXPECT_TRUE(info.pi_prot & VM_USER);
    EXPECT_OK(vm_query_page(dst, TEST_VA, &info));
    EXPECT_EQ(info.pi_phys_base, page_to_phys(pg));
    EXPECT_FALSE(info.pi_prot & VM_WRITE);

    src_vs.v_pgd = src;
    dst_vs.v_pgd = dst;
    vm_release(&src_vs, TEST_VA, SZ_4K);
    EXPECT_EQ(refcount_get(&pg->pg_refcnt), 1);
    EXPECT_FALSE(PageBuddy(pg));
    vm_release(&dst_vs, TEST_VA, SZ_4K);
    EXPECT_EQ(refcount_get(&pg->pg_refcnt), 0);

    free_pgd(state, src);
    free_pgd(state, dst);
}
