#include <sys/errno.h>
#include <sys/mm/vmspace.h>
#include <sys/mman.h>
#include <sys/proc.h>
#include <sys/string.h>
#include <sys/testing.h>

#include <asi/memory.h>
#include <asi/page.h>

#define ANON (MAP_ANONYMOUS | MAP_PRIVATE)
#define RW   (PROT_READ | PROT_WRITE)

struct range {
    virt_addr_t start, end;
};

static void check_map(struct test_state *state, struct vmspace *vs, const struct range *want, size_t n) {
    struct vm_map_entry *e;
    size_t               i = 0;

    list_for_each_entry(e, &vs->v_vmmap, vm_list) {
        if (i >= n) {
            ADD_FAILURE("unexpected extra entry [%#lx, %#lx)", e->vm_start, e->vm_end);
            return;
        }
        if (e->vm_start != want[i].start || e->vm_end != want[i].end) {
            ADD_FAILURE("entry %ld is [%#lx, %#lx), expected [%#lx, %#lx)",
                        (long) i,
                        e->vm_start,
                        e->vm_end,
                        want[i].start,
                        want[i].end);
            return;
        }
        i++;
    }

    if (i != n) { ADD_FAILURE("map has %ld entries, expected %ld", (long) i, (long) n); }
}

static struct vm_map_entry *entry_at(struct vmspace *vs, virt_addr_t va) {
    struct vm_map_entry *e;

    list_for_each_entry(e, &vs->v_vmmap, vm_list) {
        if (e->vm_start <= va && e->vm_end > va) { return e; }
    }
    return NULL;
}

KERNEL_TEST(vmspace_new_put) {
    struct vmspace *vs           = vmspace_new(NULL);
    size_t          user_entries = 0, kernel_entries = 0;

    ASSERT_NOT_NULL(vs);
    ASSERT_NOT_NULL(vs->v_pgd);
    EXPECT_EQ(refcount_get(&vs->v_refcount), 1);
    EXPECT_TRUE(list_is_empty(&vs->v_vmmap));

    for (int i = 0; i < 256; i++) {
        if (vs->v_pgd[i]) { user_entries++; }
    }
    for (int i = 256; i < 512; i++) {
        if (vs->v_pgd[i]) { kernel_entries++; }
    }
    EXPECT_EQ(user_entries, 0);
    EXPECT_EQ(kernel_entries, 256);

    vmspace_put(vs);
}

KERNEL_TEST(vmspace_share) {
    struct vmspace *vs = vmspace_new(NULL);
    struct process  p;

    ASSERT_NOT_NULL(vs);
    p.p_mm = vs;

    EXPECT_EQ(vmspace_share(&p), vs);
    EXPECT_EQ(refcount_get(&vs->v_refcount), 2);
    vmspace_put(vs);
    EXPECT_EQ(refcount_get(&vs->v_refcount), 1);
    vmspace_put(vs);
}

KERNEL_TEST(vmspace_mmap_anon) {
    struct vmspace      *vs = vmspace_new(NULL);
    struct vm_map_entry *e;
    virt_addr_t          a = 0, b = 0, c = 0;

    ASSERT_NOT_NULL(vs);

    EXPECT_OK(vms_mmap(vs, 0, 3 * PAGE_SIZE, RW, ANON, NULL, 0, &a));
    EXPECT_EQ(a, ARCH_USER_START);
    EXPECT_OK(vms_mmap(vs, 0, 5000, PROT_READ, ANON, NULL, 0, &b));
    EXPECT_EQ(b, ARCH_USER_START + 3 * PAGE_SIZE);

    EXPECT_OK(vms_mmap(vs, 0x100000, PAGE_SIZE, RW, ANON, NULL, 0, &c));
    EXPECT_EQ(c, 0x100000);

    const struct range want[] = {
            {a, a + 3 * PAGE_SIZE},
            {b, b + 2 * PAGE_SIZE},
            {0x100000, 0x101000},
    };
    check_map(state, vs, want, ARRAY_SIZE(want));

    e = entry_at(vs, b + PAGE_SIZE);
    ASSERT_NOT_NULL(e);
    EXPECT_EQ(e->vm_prot, PROT_READ);
    EXPECT_EQ(e->vm_flags, ANON);
    EXPECT_NULL(e->vm_file);

    vmspace_put(vs);
}

KERNEL_TEST(vmspace_mmap_invalid) {
    struct vmspace *vs = vmspace_new(NULL);
    virt_addr_t     a;

    ASSERT_NOT_NULL(vs);

    EXPECT_EQ(vms_mmap(vs, 0, 0, RW, ANON, NULL, 0, &a), -EINVAL);
    EXPECT_EQ(vms_mmap(vs, 0, PAGE_SIZE, RW, ANON, NULL, 123, &a), -EINVAL);
    EXPECT_EQ(vms_mmap(vs, 0x10010, PAGE_SIZE, RW, ANON | MAP_FIXED, NULL, 0, &a), -EINVAL);
    EXPECT_EQ(vms_mmap(vs, 0, SIZE_MAX, RW, ANON, NULL, 0, &a), -EINVAL);
    EXPECT_TRUE(list_is_empty(&vs->v_vmmap));

    vmspace_put(vs);
}

KERNEL_TEST(vmspace_mmap_hint_taken) {
    struct vmspace *vs = vmspace_new(NULL);
    virt_addr_t     a, b, c;

    ASSERT_NOT_NULL(vs);

    EXPECT_OK(vms_mmap(vs, 0x200000, 4 * PAGE_SIZE, RW, ANON, NULL, 0, &a));
    EXPECT_EQ(a, 0x200000);

    EXPECT_OK(vms_mmap(vs, 0x201000, PAGE_SIZE, RW, ANON, NULL, 0, &b));
    EXPECT_EQ(b, 0x204000);

    EXPECT_OK(vms_mmap(vs, 0, PAGE_SIZE, RW, ANON, NULL, 0, &c));
    EXPECT_EQ(c, ARCH_USER_START);

    const struct range want[] = {
            {ARCH_USER_START, ARCH_USER_START + PAGE_SIZE},
            {0x200000, 0x204000},
            {0x204000, 0x205000},
    };
    check_map(state, vs, want, ARRAY_SIZE(want));

    vmspace_put(vs);
}

KERNEL_TEST(vmspace_mmap_fixed_replace) {
    struct vmspace      *vs = vmspace_new(NULL);
    struct vm_map_entry *e;
    virt_addr_t          a;

    ASSERT_NOT_NULL(vs);

    EXPECT_OK(vms_mmap(vs, 0x10000, 0x10000, RW, ANON | MAP_FIXED, NULL, 0, &a));
    EXPECT_EQ(a, 0x10000);

    EXPECT_OK(vms_mmap(vs, 0x14000, 0x4000, PROT_READ, ANON | MAP_FIXED, NULL, 0, &a));
    EXPECT_EQ(a, 0x14000);

    const struct range split[] = {
            {0x10000, 0x14000},
            {0x14000, 0x18000},
            {0x18000, 0x20000},
    };
    ASSERT_NO_FAILURE(check_map(state, vs, split, ARRAY_SIZE(split)));

    e = entry_at(vs, 0x14000);
    ASSERT_NOT_NULL(e);
    EXPECT_EQ(e->vm_prot, PROT_READ);
    e = entry_at(vs, 0x18000);
    ASSERT_NOT_NULL(e);
    EXPECT_EQ(e->vm_prot, RW);
    EXPECT_EQ(e->vm_foff, 0x8000);

    EXPECT_OK(vms_mmap(vs, 0x12000, 0x8000, RW, ANON | MAP_FIXED, NULL, 0, &a));
    const struct range trimmed[] = {
            {0x10000, 0x12000},
            {0x12000, 0x1A000},
            {0x1A000, 0x20000},
    };
    check_map(state, vs, trimmed, ARRAY_SIZE(trimmed));

    vmspace_put(vs);
}

KERNEL_TEST(vmspace_munmap) {
    struct vmspace *vs = vmspace_new(NULL);
    virt_addr_t     a;

    ASSERT_NOT_NULL(vs);
    EXPECT_OK(vms_mmap(vs, 0x10000, 0x10000, RW, ANON | MAP_FIXED, NULL, 0, &a));

    EXPECT_OK(vms_munmap(vs, 0x10000, 0x1000));
    EXPECT_OK(vms_munmap(vs, 0x1F000, 0x1000));
    EXPECT_OK(vms_munmap(vs, 0x14000, 0x2000));
    const struct range want[] = {
            {0x11000, 0x14000},
            {0x16000, 0x1F000},
    };
    ASSERT_NO_FAILURE(check_map(state, vs, want, ARRAY_SIZE(want)));

    EXPECT_OK(vms_munmap(vs, 0x40000, 0x1000));

    EXPECT_OK(vms_munmap(vs, 0x10000, 0x10000));
    EXPECT_TRUE(list_is_empty(&vs->v_vmmap));

    EXPECT_EQ(vms_munmap(vs, 0x10010, 0x1000), -EINVAL);
    EXPECT_EQ(vms_munmap(vs, 0x10000, 0), -EINVAL);
    EXPECT_EQ(vms_munmap(vs, ARCH_USER_END + 1, 0x1000), -EINVAL);
    EXPECT_EQ(vms_munmap(vs, 0x10000, ARCH_USER_END), -EINVAL);

    vmspace_put(vs);
}

KERNEL_TEST(vmspace_fork) {
    struct vmspace *vs = vmspace_new(NULL);
    struct vmspace *child;
    struct process  p;
    virt_addr_t     a;

    ASSERT_NOT_NULL(vs);
    EXPECT_OK(vms_mmap(vs, 0x10000, 0x4000, RW, ANON | MAP_FIXED, NULL, 0, &a));
    EXPECT_OK(vms_mmap(vs, 0x30000, 0x2000, PROT_READ, ANON | MAP_FIXED, NULL, 0, &a));

    p.p_mm = vs;
    child  = vmspace_fork(&p);
    ASSERT_NOT_NULL(child);
    EXPECT_NE(child, vs);
    EXPECT_NE(child->v_pgd, vs->v_pgd);

    const struct range want[] = {
            {0x10000, 0x14000},
            {0x30000, 0x32000},
    };
    ASSERT_NO_FAILURE(check_map(state, child, want, ARRAY_SIZE(want)));
    EXPECT_EQ(entry_at(child, 0x30000)->vm_prot, PROT_READ);

    /* the maps are independent */
    EXPECT_OK(vms_munmap(child, 0x10000, 0x4000));
    check_map(state, vs, want, ARRAY_SIZE(want));

    vmspace_put(child);
    vmspace_put(vs);
}

KERNEL_TEST(vmspace_access_ok) {
    EXPECT_TRUE(access_ok((void *) 0x10000, 0x1000));
    EXPECT_TRUE(access_ok((void *) (ARCH_USER_END - 0xFFF), 0x1000));

    EXPECT_FALSE(access_ok((void *) ARCH_DIRECT_MAP_BASE, 16));
    EXPECT_FALSE(access_ok((void *) ARCH_KERNEL_BASE, 1));
    EXPECT_FALSE(access_ok((void *) (ARCH_USER_END - 0xFFF), 0x2000));
    EXPECT_FALSE(access_ok((void *) 0x10000, ~0ul));
}
