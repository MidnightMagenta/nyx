#include <mm/pmm.h>
#include <nyx/testing.h>

KERNEL_TEST(pmm_sanity) {
    size_t      free, used, total, old_total;
    phys_addr_t alloc;

    free      = pm_getstat(PM_STAT_FREE);
    used      = pm_getstat(PM_STAT_USED);
    total     = pm_getstat(PM_STAT_TOTAL);
    old_total = total;

    EXPECT_EQ(free + used, total);

    alloc = __pm_get_pages(0, 0);

    free  = pm_getstat(PM_STAT_FREE);
    used  = pm_getstat(PM_STAT_USED);
    total = pm_getstat(PM_STAT_TOTAL);

    EXPECT_EQ(free + used, total);
    EXPECT_EQ(total, old_total);

    pm_free_pages(alloc);

    free  = pm_getstat(PM_STAT_FREE);
    used  = pm_getstat(PM_STAT_USED);
    total = pm_getstat(PM_STAT_TOTAL);

    EXPECT_EQ(free + used, total);
    EXPECT_EQ(total, old_total);
}

KERNEL_TEST(pmm_alloc_free) {
    struct page *alloc;
    size_t       initial_free_cnt = pm_getstat(PM_STAT_FREE);

    alloc = __pm_alloc_pages(0, 0);

    EXPECT_TRUE(alloc);
    EXPECT_EQ(pm_getstat(PM_STAT_FREE), initial_free_cnt - 1);
    EXPECT_TRUE(alloc->flags & PAGE_FLAG_HEAD);

    pm_free_pages(pm_page_to_phys(alloc));

    EXPECT_EQ(pm_getstat(PM_STAT_FREE), initial_free_cnt);
    EXPECT_FALSE(alloc->flags & PAGE_FLAG_HEAD);
}
