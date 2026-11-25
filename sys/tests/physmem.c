#include <sys/mm/address.h>
#include <sys/mm/physmem.h>
#include <sys/string.h>
#include <sys/testing.h>

#include <asi/page.h>

static void check_zone(struct test_state *state, zone_t *zone) {
    size_t free_pages = 0;

    for (unsigned long order = 0; order < MAX_ORDER; order++) {
        struct free_area_s *area = &zone->free_area[order];
        struct page        *pg;
        size_t              n = 0;

        list_for_each_entry(pg, &area->list[0], pg_list) {
            size_t idx = (size_t) (pg - zone->zone_mem_map);

            if (!PageBuddy(pg) || pg->pg_private != order) {
                ADD_FAILURE("zone %s order %ld: free block %ld not marked as buddy of its order",
                            zone->name,
                            (long) order,
                            (long) idx);
                return;
            }
            if (idx & ((1ul << order) - 1)) {
                ADD_FAILURE("zone %s order %ld: free block %ld misaligned", zone->name, (long) order, (long) idx);
                return;
            }
            if (idx + (1ul << order) > zone->spanned_pages) {
                ADD_FAILURE("zone %s order %ld: free block %ld out of zone", zone->name, (long) order, (long) idx);
                return;
            }
            n++;
        }

        EXPECT_EQ(n, area->free_count);
        free_pages += n << order;
    }

    EXPECT_EQ(free_pages, zone->free_pages);
}

static void check_all_zones(struct test_state *state) {
    for (int i = 0; i < MAX_NR_ZONES; i++) {
        if (pgdata->zones[i].spanned_pages) { check_zone(state, &pgdata->zones[i]); }
    }
}

static zone_t *page_zone(struct page *pg) {
    return &pgdata->zones[pg->pg_zone_id];
}

KERNEL_TEST(physmem_round_trip) {
    struct page *first_page, *second_page;
    first_page = pm_alloc_pages(0, 0);
    ASSERT_NOT_NULL(first_page);
    __pm_free_pages(first_page, 0);
    second_page = pm_alloc_pages(0, 0);
    ASSERT_EQ(first_page, second_page);

    __pm_free_pages(second_page, 0);
}

KERNEL_TEST(physmem_zone_invariants) {
    ASSERT_NO_FAILURE(check_all_zones(state));
}

KERNEL_TEST(physmem_alloc_orders) {
    struct page *pages[MAX_ORDER];

    for (unsigned long order = 0; order < MAX_ORDER; order++) {
        struct page *pg = pm_alloc_pages(0, order);
        zone_t      *zone;
        size_t       idx;

        pages[order] = pg;
        if (!pg) {
            ADD_FAILURE("order %ld allocation failed", (long) order);
            continue;
        }

        zone = page_zone(pg);
        idx  = (size_t) (pg - zone->zone_mem_map);
        EXPECT_TRUE(PageHead(pg));
        EXPECT_FALSE(PageBuddy(pg));
        EXPECT_EQ(pg->pg_head_order, (int) order);
        EXPECT_EQ(refcount_get(&pg->pg_refcnt), 0);
        EXPECT_EQ(idx & ((1ul << order) - 1), 0);
        EXPECT_EQ(page_to_pfn(pg) & ((1ul << order) - 1), 0);
    }

    for (unsigned long order = 0; order < MAX_ORDER; order++) {
        if (pages[order]) { __pm_free_pages(pages[order], order); }
    }

    check_all_zones(state);
}

KERNEL_TEST(physmem_free_accounting) {
    struct page *pg;
    zone_t      *zone;
    size_t       after_alloc;

    pg = pm_alloc_pages(0, 3);
    ASSERT_NOT_NULL(pg);
    zone        = page_zone(pg);
    after_alloc = zone->free_pages;

    __pm_free_pages(pg, 3);
    EXPECT_EQ(zone->free_pages, after_alloc + 8);
    EXPECT_FALSE(PageHead(pg));

    pg = pm_alloc_pages(0, 3);
    ASSERT_NOT_NULL(pg);
    EXPECT_EQ(page_zone(pg), zone);
    EXPECT_EQ(zone->free_pages, after_alloc);
    __pm_free_pages(pg, 3);

    check_all_zones(state);
}

KERNEL_TEST(physmem_many_allocs) {
    static struct page *pages[256];
    size_t              before[MAX_NR_ZONES];

    for (int i = 0; i < MAX_NR_ZONES; i++) { before[i] = pgdata->zones[i].free_pages; }

    for (int i = 0; i < 256; i++) {
        pages[i] = pm_alloc_pages(0, (unsigned long) (i % 3));
        if (!pages[i]) {
            ADD_FAILURE("allocation %d failed", i);
            break;
        }
        for (int j = 0; j < i; j++) {
            if (pages[j] == pages[i]) {
                ADD_FAILURE("allocations %d and %d returned the same block", j, i);
                break;
            }
        }
    }

    for (int i = 0; i < 256; i += 2) {
        if (pages[i]) { __pm_free_pages(pages[i], (unsigned long) (i % 3)); }
    }
    for (int i = 1; i < 256; i += 2) {
        if (pages[i]) { __pm_free_pages(pages[i], (unsigned long) (i % 3)); }
    }

    for (int i = 0; i < MAX_NR_ZONES; i++) { EXPECT_EQ(pgdata->zones[i].free_pages, before[i]); }
    check_all_zones(state);
}

KERNEL_TEST(physmem_address_conversion) {
    struct page *pg = pm_alloc_pages(0, 0);
    phys_addr_t  pa;
    void        *va;

    ASSERT_NOT_NULL(pg);
    pa = page_to_phys(pg);
    va = page_address(pg);

    EXPECT_EQ(pa & (PAGE_SIZE - 1), 0);
    EXPECT_EQ(phys_to_page(pa), pg);
    EXPECT_EQ(pfn_to_page(page_to_pfn(pg)), pg);
    EXPECT_EQ(virt_to_page(va), pg);
    EXPECT_EQ(virt_to_page((char *) va + PAGE_SIZE - 1), pg);
    EXPECT_EQ((phys_addr_t) __pa(va), pa);

    memset(va, 0x5A, PAGE_SIZE);
    EXPECT_EQ(((unsigned char *) va)[PAGE_SIZE - 1], 0x5A);

    pm_free_pages(pa, 0);
    EXPECT_FALSE(PageHead(pg));
}

KERNEL_TEST(physmem_zeroed_page) {
    struct page   *pg;
    unsigned char *va;
    phys_addr_t    pa;
    size_t         dirty = 0;

    pg = pm_alloc_pages(0, 0);
    ASSERT_NOT_NULL(pg);
    memset(page_address(pg), 0xA5, PAGE_SIZE);
    __pm_free_pages(pg, 0);

    pa = pm_get_zeroed_page(0);
    ASSERT_NE(pa, INVALID_PHYS_ADDR);
    va = __va(pa);
    for (size_t i = 0; i < PAGE_SIZE; i++) {
        if (va[i]) { dirty++; }
    }
    EXPECT_EQ(dirty, 0);

    pm_free_pages(pa, 0);
}

KERNEL_TEST(physmem_zeroed_block) {
    struct page   *pg;
    unsigned char *va;
    size_t         dirty = 0;

    pg = pm_alloc_pages(0, 2);
    ASSERT_NOT_NULL(pg);
    memset(page_address(pg), 0xA5, 4 * PAGE_SIZE);
    __pm_free_pages(pg, 2);

    pg = pm_alloc_pages(__M_ZERO, 2);
    ASSERT_NOT_NULL(pg);
    va = page_address(pg);
    for (size_t i = 0; i < 4 * PAGE_SIZE; i++) {
        if (va[i]) { dirty++; }
    }
    EXPECT_EQ(dirty, 0);

    __pm_free_pages(pg, 2);
}

KERNEL_TEST(physmem_zone_selection) {
    struct page *pg;

    pg = pm_alloc_pages(M_DMA, 0);
    ASSERT_NOT_NULL(pg);
    EXPECT_EQ(pg->pg_zone_id, ZONE_DMA);
    EXPECT_LT(page_to_phys(pg), 16ul << 20);
    __pm_free_pages(pg, 0);

    pg = pm_alloc_pages(M_DMA32, 0);
    ASSERT_NOT_NULL(pg);
    EXPECT_LE(pg->pg_zone_id, ZONE_DMA32);
    EXPECT_LT(page_to_phys(pg), 4ul << 30);
    __pm_free_pages(pg, 0);
}

KERNEL_TEST(physmem_invalid_order) {
    EXPECT_NULL(pm_alloc_pages(0, MAX_ORDER));
    EXPECT_EQ(__pm_get_free_pages(0, MAX_ORDER), INVALID_PHYS_ADDR);
    EXPECT_EQ(__pm_get_free_pages(0, MAX_ORDER + 5), INVALID_PHYS_ADDR);
}
