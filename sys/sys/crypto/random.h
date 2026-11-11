#ifndef _CRYPTO_RANDOM_H
#define _CRYPTO_RANDOM_H

#include <sys/stddef.h>
#include <sys/types.h>

void rng_get_bytes(void *buf, size_t len);
u64  rng_get_u64();
bool rng_is_seeded();

void rng_add_device_entropy(const void *buf, size_t len);
void rng_add_hwrng(const void *buf, size_t len);

#endif
