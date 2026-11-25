#include <sys/align.h>
#include <sys/atomic.h>
#include <sys/bitmap.h>
#include <sys/kernel.h>
#include <sys/minmax.h>
#include <sys/refcount.h>
#include <sys/testing.h>
#include <sys/vfs.h>

#include <asi/bitops.h>

KERNEL_TEST(bits_align) {
    EXPECT_EQ(ALIGN_UP(0, 8), 0);
    EXPECT_EQ(ALIGN_UP(1, 8), 8);
    EXPECT_EQ(ALIGN_UP(8, 8), 8);
    EXPECT_EQ(ALIGN_UP(9, 8), 16);
    EXPECT_EQ(ALIGN_DOWN(0, 8), 0);
    EXPECT_EQ(ALIGN_DOWN(7, 8), 0);
    EXPECT_EQ(ALIGN_DOWN(15, 8), 8);
    EXPECT_EQ(ALIGN_UP(0x1001ul, 0x1000ul), 0x2000ul);
    EXPECT_EQ(ALIGN_DOWN(0xFFFFFFFFFFFFFFFFul, 0x1000ul), 0xFFFFFFFFFFFFF000ul);

    EXPECT_EQ(MIN(3, 5), 3);
    EXPECT_EQ(MAX(3, 5), 5);
    EXPECT_EQ(MIN(-1, 1), -1);
    EXPECT_EQ(MAX(-1, 1), 1);

    static const unsigned long pow2[]     = {1, 2, 4096, 1ul << 63};
    static const unsigned long not_pow2[] = {0, 3, 6, 4097, ~0ul};
    for (size_t i = 0; i < ARRAY_SIZE(pow2); i++) { EXPECT_TRUE(IS_POWER_OF_TWO(pow2[i])); }
    for (size_t i = 0; i < ARRAY_SIZE(not_pow2); i++) { EXPECT_FALSE(IS_POWER_OF_TWO(not_pow2[i])); }

    int arr[7];
    EXPECT_EQ(ARRAY_SIZE(arr), 7);
}

KERNEL_TEST(bits_ilog2) {
    EXPECT_EQ(ilog2(1u), 0);
    EXPECT_EQ(ilog2(2u), 1);
    EXPECT_EQ(ilog2(3u), 1);
    EXPECT_EQ(ilog2(4096u), 12);
    EXPECT_EQ(ilog2(0xFFFFFFFFu), 31);
    EXPECT_EQ(ilog2(1ul << 40), 40);
    EXPECT_EQ(ilog2(~0ul), 63);
    EXPECT_EQ(ilog2(1ull << 50), 50);

    EXPECT_EQ(cilog2(0ul), 0);
    EXPECT_EQ(cilog2(1ul), 0);
    EXPECT_EQ(cilog2(2ul), 1);
    EXPECT_EQ(cilog2(3ul), 2);
    EXPECT_EQ(cilog2(4096ul), 12);
    EXPECT_EQ(cilog2(4097ul), 13);

    EXPECT_EQ(BIT(0), 1ull);
    EXPECT_EQ(BIT(63), 0x8000000000000000ull);
}

KERNEL_TEST(bits_bitops) {
    unsigned long long words[2] = {0, 0};

    set_bit(0, words);
    set_bit(63, words);
    set_bit(64, words);
    set_bit(100, words);
    EXPECT_EQ(words[0], 0x8000000000000001ull);
    EXPECT_EQ(words[1], (1ull << 0) | (1ull << 36));

    EXPECT_TRUE(test_bit(0, words));
    EXPECT_TRUE(test_bit(63, words));
    EXPECT_TRUE(test_bit(64, words));
    EXPECT_TRUE(test_bit(100, words));
    EXPECT_FALSE(test_bit(1, words));
    EXPECT_FALSE(test_bit(101, words));

    clear_bit(63, words);
    clear_bit(100, words);
    EXPECT_EQ(words[0], 1ull);
    EXPECT_EQ(words[1], 1ull);
    EXPECT_FALSE(test_bit(63, words));
}

KERNEL_TEST(bits_bitmap) {
    DECLARE_STATIC_BITMAP(bm, 130);

    EXPECT_EQ(sizeof(bm), 3 * sizeof(bitmap_word_t));
    memset(bm, 0, sizeof(bm));

    bm_set(bm, 0);
    bm_set(bm, 64);
    bm_set(bm, 129);
    EXPECT_EQ(bm_get(bm, 0), 1);
    EXPECT_EQ(bm_get(bm, 1), 0);
    EXPECT_EQ(bm_get(bm, 64), 1);
    EXPECT_EQ(bm_get(bm, 129), 1);
    EXPECT_EQ(bm[2], 2ull);

    bm_clear(bm, 64);
    EXPECT_EQ(bm_get(bm, 64), 0);
    EXPECT_EQ(bm[1], 0ull);

    bm_flip(bm, 5);
    bm_flip(bm, 0);
    EXPECT_EQ(bm[0], 1ull << 5);
    bm_flip(bm, 5);
    EXPECT_EQ(bm[0], 0ull);
}

KERNEL_TEST(bits_load_store_le) {
    const unsigned char bytes[16] = {
            0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0xF0, 0xE0, 0xD0, 0xC0, 0xB0, 0xA0, 0x90, 0x80,
    };
    unsigned char out[16];
    u16           w[2];
    u32           d[2];
    u64           q[2];

    load_le_u16(w, bytes, 2);
    EXPECT_EQ(w[0], 0x0201);
    EXPECT_EQ(w[1], 0x0403);

    load_le_u32(d, bytes, 2);
    EXPECT_EQ(d[0], 0x04030201u);
    EXPECT_EQ(d[1], 0x08070605u);

    load_le_u64(q, bytes, 2);
    EXPECT_EQ(q[0], 0x0807060504030201ull);
    EXPECT_EQ(q[1], 0x8090A0B0C0D0E0F0ull);

    memset(out, 0, sizeof(out));
    store_le_u64(out, q, 2);
    EXPECT_MEMEQ(out, bytes, 16);

    memset(out, 0, sizeof(out));
    store_le_u32(out, d, 2);
    EXPECT_MEMEQ(out, bytes, 8);

    memset(out, 0, sizeof(out));
    store_le_u16(out, w, 2);
    EXPECT_MEMEQ(out, bytes, 4);
}

KERNEL_TEST(bits_makedev) {
    dev_t dev = makedev(200, 17);

    EXPECT_EQ(major(dev), 200);
    EXPECT_EQ(minor(dev), 17);
    EXPECT_EQ(major(makedev(1, MINORMASK)), 1);
    EXPECT_EQ(minor(makedev(1, MINORMASK)), MINORMASK);
    EXPECT_EQ(minor(makedev(1, MINORMASK + 1)), 0);
    EXPECT_NE(makedev(1, 3), makedev(3, 1));
}

KERNEL_TEST(atomic_ops) {
    atomic_int_t   a;
    atomic_ulong_t u;
    atomic_bool_t  flag;

    atomic_store(&a, 5);
    EXPECT_EQ(atomic_load(&a), 5);
    EXPECT_EQ(atomic_fetch_add(&a, 3, ATOMIC_SEQ_CST), 5);
    EXPECT_EQ(atomic_load(&a), 8);
    EXPECT_EQ(atomic_fetch_sub(&a, 10, ATOMIC_SEQ_CST), 8);
    EXPECT_EQ(atomic_load(&a), -2);
    EXPECT_EQ(atomic_inc_e(&a, ATOMIC_RELAXED), -2);
    EXPECT_EQ(atomic_dec_e(&a, ATOMIC_RELAXED), -1);
    EXPECT_EQ(atomic_load(&a), -2);

    atomic_store(&u, 0xF0ul);
    EXPECT_EQ(atomic_fetch_or(&u, 0x0Ful, ATOMIC_SEQ_CST), 0xF0ul);
    EXPECT_EQ(atomic_fetch_and(&u, 0x3Cul, ATOMIC_SEQ_CST), 0xFFul);
    EXPECT_EQ(atomic_fetch_xor(&u, 0xFFul, ATOMIC_SEQ_CST), 0x3Cul);
    EXPECT_EQ(atomic_load(&u), 0xC3ul);

    atomic_clear(&flag, ATOMIC_SEQ_CST);
    EXPECT_FALSE(atomic_test_and_set(&flag, ATOMIC_SEQ_CST));
    EXPECT_TRUE(atomic_test_and_set(&flag, ATOMIC_SEQ_CST));
    atomic_clear(&flag, ATOMIC_SEQ_CST);
    EXPECT_FALSE(atomic_test_and_set(&flag, ATOMIC_SEQ_CST));
}

KERNEL_TEST(refcount_ops) {
    refcount_t rc;

    refcount_init(&rc, 1);
    EXPECT_EQ(refcount_get(&rc), 1);

    refcount_inc(&rc);
    EXPECT_EQ(refcount_get(&rc), 2);
    EXPECT_EQ(refcount_get_inc(&rc), 2);
    EXPECT_EQ(refcount_get_dec(&rc), 3);

    EXPECT_FALSE(refcount_dec_and_test(&rc));
    EXPECT_EQ(refcount_get(&rc), 1);
    EXPECT_TRUE(refcount_dec_and_test(&rc));
    EXPECT_EQ(refcount_get(&rc), 0);

    refcount_set(&rc, 10);
    refcount_dec(&rc);
    EXPECT_EQ(refcount_get(&rc), 9);
}
