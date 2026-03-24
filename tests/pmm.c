#include <mm/pmm.h>
#include <nyx/testing.h>

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
