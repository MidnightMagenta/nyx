#include <sys/current.h>
#include <sys/mutex.h>
#include <sys/spinlock.h>
#include <sys/testing.h>

#include <asi/irq.h>

#define RFLAGS_IF (1ul << 9)

static flags_t read_flags() {
    flags_t flags = arch_irq_save();
    arch_irq_restore(flags);
    return flags;
}

KERNEL_TEST(spinlock_basic) {
    spinlock_t lock;
    flags_t    before = read_flags();

    spinlock_init(&lock);
    EXPECT_FALSE(atomic_load(&lock.__locked));

    spin_lock(&lock);
    EXPECT_TRUE(atomic_load(&lock.__locked));
    EXPECT_EQ(read_flags() & RFLAGS_IF, 0);
    spin_unlock(&lock);

    EXPECT_FALSE(atomic_load(&lock.__locked));
    EXPECT_EQ(read_flags(), before);
}

KERNEL_TEST(spinlock_nested) {
    spinlock_t a, b;
    flags_t    before = read_flags();

    spinlock_init(&a);
    spinlock_init(&b);

    spin_lock(&a);
    spin_lock(&b);
    EXPECT_TRUE(atomic_load(&a.__locked));
    EXPECT_TRUE(atomic_load(&b.__locked));
    spin_unlock(&b);
    EXPECT_FALSE(atomic_load(&b.__locked));
    EXPECT_EQ(read_flags() & RFLAGS_IF, 0);
    spin_unlock(&a);

    EXPECT_FALSE(atomic_load(&a.__locked));
    EXPECT_EQ(read_flags(), before);
}

KERNEL_TEST(mutex_uncontended) {
    struct mutex m;

    mutex_init(&m);
    EXPECT_FALSE(m.m_locked);
    EXPECT_NULL(m.m_owner);

    mutex_lock(&m);
    EXPECT_TRUE(m.m_locked);
    EXPECT_EQ(m.m_owner, current());
    EXPECT_FALSE(atomic_load(&m.m_lock.__locked));
    mutex_unlock(&m);

    EXPECT_FALSE(m.m_locked);
    EXPECT_NULL(m.m_owner);
    EXPECT_EQ(m.m_waiters, 0);
}

KERNEL_TEST(mutex_trylock) {
    struct mutex m;

    mutex_init(&m);
    ASSERT_TRUE(mutex_trylock(&m));
    EXPECT_EQ(m.m_owner, current());
    EXPECT_FALSE(mutex_trylock(&m));
    EXPECT_TRUE(m.m_locked);
    mutex_unlock(&m);

    EXPECT_TRUE(mutex_trylock(&m));
    mutex_unlock(&m);

    mutex_lock(&m);
    EXPECT_FALSE(mutex_trylock(&m));
    mutex_unlock(&m);
    EXPECT_FALSE(m.m_locked);
}
