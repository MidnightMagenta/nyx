#ifndef _CRYPTO_CRYPTO_H
#define _CRYPTO_CRYPTO_H

#include <nyx/stddef.h>
#include <nyx/types.h>

void chacha20_block(const u32 key[8], u32 counter, const u32 nonce[3], u32 out[16]);

typedef struct {
    u32    h[8];
    u8     buf[64];
    size_t buflen;
    u64    t;
    size_t outlen;
} blake2s_ctx;

void blake2s_init(blake2s_ctx *c, size_t outlen);
void blake2s_compress(u32 h[8], const u32 m[16], u64 t, int las);
void blake2s_update(blake2s_ctx *c, const u8 *in, size_t n);
void blake2s_final(blake2s_ctx *c, u8 *out);

static inline void blake2s_256(u8 out[32], const void *in, size_t inlen) {
    blake2s_ctx c;
    blake2s_init(&c, 32);
    blake2s_update(&c, in, inlen);
    blake2s_final(&c, out);
}

#endif
