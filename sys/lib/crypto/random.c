#include <asi/tsc.h>
#include <crypto/monocypher.h>
#include <crypto/random.h>
#include <nyx/types.h>

static struct {
    crypto_blake2b_ctx pool;
    u8                 rng_key[32];
    bool               seeded;
    size_t             hwrng_bytes_seen;
} random_ctx;

static void rng_reseed() {
    u8                 out[32];
    crypto_blake2b_ctx copy, k;

    copy = random_ctx.pool;
    crypto_blake2b_final(&copy, out);

    crypto_blake2b_init(&k, 32);
    crypto_blake2b_update(&k, random_ctx.rng_key, 32);
    crypto_blake2b_update(&k, out, 32);
    crypto_blake2b_final(&k, random_ctx.rng_key);

    crypto_blake2b_update(&random_ctx.pool, out, 32);

    crypto_wipe(out, sizeof(out));
    crypto_wipe(&copy, sizeof(copy));
    crypto_wipe(&k, sizeof(k));
}

static void pool_absorb(u8 tag, const void *buf, size_t len) {
    u64 tsc = read_tsc();

    crypto_blake2b_update(&random_ctx.pool, &tag, 1);
    crypto_blake2b_update(&random_ctx.pool, (const u8 *) &len, sizeof(len));
    crypto_blake2b_update(&random_ctx.pool, buf, len);
    crypto_blake2b_update(&random_ctx.pool, (const u8 *) tsc, sizeof(tsc));
}
