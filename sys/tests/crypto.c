#include <sys/crypto/monocypher.h>
#include <sys/string.h>
#include <sys/testing.h>

static int unhex(const char *s, u8 *out, size_t n) {
    for (size_t i = 0; i < n; i++) {
        int v = 0;
        for (int j = 0; j < 2; j++) {
            char c = s[2 * i + j];
            v <<= 4;
            if (c >= '0' && c <= '9') {
                v |= c - '0';
            } else if (c >= 'a' && c <= 'f') {
                v |= c - 'a' + 10;
            } else {
                return -1;
            }
        }
        out[i] = (u8) v;
    }
    return 0;
}

/* RFC 7693, Appendix A */
static const char blake2b_abc[] = "ba80a53f981c4d0d6a2797b69f12f6e94c212f14685ac4b74b12bb6fdbffa2d1"
                                  "7d87c5392aab792dc252d5de4533cc9518d38aa8dbf1925ab92386edd4009923";

static const char blake2b_empty[] = "786a02f742015903c6c6fd852552d272912f4740e15847618a86e217f71f5419"
                                    "d25e1031afee585313896444934eb04b903a685b1448b755d56f701afe9be2ce";

KERNEL_TEST(crypto_blake2b_vectors) {
    u8 want[64], got[64];

    ASSERT_OK(unhex(blake2b_abc, want, 64));
    crypto_blake2b(got, 64, (const u8 *) "abc", 3);
    EXPECT_MEMEQ(got, want, 64);

    ASSERT_OK(unhex(blake2b_empty, want, 64));
    crypto_blake2b(got, 64, NULL, 0);
    EXPECT_MEMEQ(got, want, 64);
}

KERNEL_TEST(crypto_blake2b_incremental) {
    static u8          msg[1000];
    u8                 one_shot[32], chunked[32];
    crypto_blake2b_ctx ctx;

    for (size_t i = 0; i < sizeof(msg); i++) { msg[i] = (u8) (i * 31 + 7); }

    crypto_blake2b(one_shot, 32, msg, sizeof(msg));

    crypto_blake2b_init(&ctx, 32);
    for (size_t off = 0, step = 1; off < sizeof(msg); off += step, step = step * 2 + 1) {
        size_t n = sizeof(msg) - off < step ? sizeof(msg) - off : step;
        crypto_blake2b_update(&ctx, msg + off, n);
    }
    crypto_blake2b_final(&ctx, chunked);

    EXPECT_MEMEQ(one_shot, chunked, 32);

    msg[500] ^= 1;
    crypto_blake2b(chunked, 32, msg, sizeof(msg));
    EXPECT_NE(memcmp(one_shot, chunked, 32), 0);
}

KERNEL_TEST(crypto_blake2b_keyed) {
    u8 k1[32], k2[32], h1[32], h2[32], h3[32];

    memset(k1, 0x11, sizeof(k1));
    memset(k2, 0x22, sizeof(k2));

    crypto_blake2b_keyed(h1, 32, k1, 32, (const u8 *) "message", 7);
    crypto_blake2b_keyed(h2, 32, k1, 32, (const u8 *) "message", 7);
    crypto_blake2b_keyed(h3, 32, k2, 32, (const u8 *) "message", 7);

    EXPECT_MEMEQ(h1, h2, 32);
    EXPECT_NE(memcmp(h1, h3, 32), 0);
}

KERNEL_TEST(crypto_verify_wipe) {
    u8 a[64], b[64];

    for (int i = 0; i < 64; i++) { a[i] = b[i] = (u8) i; }

    EXPECT_EQ(crypto_verify16(a, b), 0);
    EXPECT_EQ(crypto_verify32(a, b), 0);
    EXPECT_EQ(crypto_verify64(a, b), 0);

    b[15] ^= 0x80;
    EXPECT_EQ(crypto_verify16(a, b), -1);
    EXPECT_EQ(crypto_verify32(a, b), -1);
    b[15] ^= 0x80;
    b[63] ^= 1;
    EXPECT_EQ(crypto_verify16(a, b), 0);
    EXPECT_EQ(crypto_verify32(a, b), 0);
    EXPECT_EQ(crypto_verify64(a, b), -1);

    crypto_wipe(a, sizeof(a));
    for (int i = 0; i < 64; i++) { EXPECT_EQ(a[i], 0); }
}
