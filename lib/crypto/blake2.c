#include <crypto/crypto.h>
#include <nyx/kernel.h>
#include <nyx/string.h>
#include <nyx/types.h>

#include <asi/bitops.h>

#define ROTR(x, n) (((x) >> (n)) | ((x) << (32 - (n))))

static const u32 IV[8] =
        {0x6A09E667, 0xBB67AE85, 0x3C6EF372, 0xA54FF53A, 0x510E527F, 0x9B05688C, 0x1F83D9AB, 0x5BE0CD19};

static const u8 SIGMA[10][16] = {
        {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15},
        {14, 10, 4, 8, 9, 15, 13, 6, 1, 12, 0, 2, 11, 7, 5, 3},
        {11, 8, 12, 0, 5, 2, 15, 13, 10, 14, 3, 6, 7, 1, 9, 4},
        {7, 9, 3, 1, 13, 12, 11, 14, 2, 6, 5, 10, 4, 0, 15, 8},
        {9, 0, 5, 7, 2, 4, 10, 15, 14, 1, 11, 12, 6, 8, 3, 13},
        {2, 12, 6, 10, 0, 11, 8, 3, 4, 13, 7, 5, 15, 14, 1, 9},
        {12, 5, 1, 15, 14, 13, 4, 10, 0, 7, 6, 3, 9, 2, 8, 11},
        {13, 11, 7, 14, 12, 1, 3, 9, 5, 0, 15, 4, 8, 6, 2, 10},
        {6, 15, 14, 9, 11, 3, 0, 8, 12, 2, 13, 7, 1, 4, 10, 5},
        {10, 2, 8, 4, 7, 6, 1, 5, 15, 11, 9, 14, 3, 12, 13, 0},
};

#define G(a, b, c, d, x, y)                                                                                            \
    do {                                                                                                               \
        a += b + x;                                                                                                    \
        d = ROTR(d ^ a, 16);                                                                                           \
        c += d;                                                                                                        \
        b = ROTR(b ^ c, 12);                                                                                           \
        a += b + y;                                                                                                    \
        d = ROTR(d ^ a, 8);                                                                                            \
        c += d;                                                                                                        \
        b = ROTR(b ^ c, 7);                                                                                            \
    } while (0)

void blake2s_init(blake2s_ctx *c, size_t outlen) {
    memcpy(c->h, IV, sizeof(IV));
    c->h[0] ^= 0x01010000 ^ (u32) outlen;
    c->buflen = 0;
    c->t      = 0;
    c->outlen = outlen;
}

void blake2s_compress(u32 h[8], const u32 m[16], u64 t, int last) {
    u32 v[16];
    for (int i = 0; i < 8; i++) {
        v[i]     = h[i];
        v[i + 8] = IV[i];
    }
    v[12] ^= (u32) t;
    v[13] ^= (u32) (t >> 32);
    if (last) v[14] = ~v[14];

    for (int r = 0; r < 10; r++) {
        const u8 *s = SIGMA[r];
        G(v[0], v[4], v[8], v[12], m[s[0]], m[s[1]]);
        G(v[1], v[5], v[9], v[13], m[s[2]], m[s[3]]);
        G(v[2], v[6], v[10], v[14], m[s[4]], m[s[5]]);
        G(v[3], v[7], v[11], v[15], m[s[6]], m[s[7]]);
        G(v[0], v[5], v[10], v[15], m[s[8]], m[s[9]]);
        G(v[1], v[6], v[11], v[12], m[s[10]], m[s[11]]);
        G(v[2], v[7], v[8], v[13], m[s[12]], m[s[13]]);
        G(v[3], v[4], v[9], v[14], m[s[14]], m[s[15]]);
    }
    for (int i = 0; i < 8; i++) h[i] ^= v[i] ^ v[i + 8];
}

void blake2s_update(blake2s_ctx *c, const u8 *in, size_t n) {
    while (n) {
        if (c->buflen == 64) {
            c->t += 64;
            u32 m[16];
            load_le_u32(m, c->buf, ARRAY_SIZE(m));
            blake2s_compress(c->h, m, c->t, 0);
            c->buflen = 0;
        }
        size_t take = 64 - c->buflen;
        if (take > n) { take = n; }
        memcpy(c->buf + c->buflen, in, take);
        c->buflen += take;
        in += take;
        n -= take;
    }
}

void blake2s_final(blake2s_ctx *c, u8 *out) {
    c->t += c->buflen;
    memset(c->buf + c->buflen, 0, 64 - c->buflen);
    u32 m[16];
    load_le_u32(m, c->buf, ARRAY_SIZE(m));
    blake2s_compress(c->h, m, c->t, 1);
    u8 full[32];
    store_le_u32(full, c->h, ARRAY_SIZE(c->h));
    memcpy(out, full, c->outlen);
}
