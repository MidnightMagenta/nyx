#ifndef _CRYPTO_CHACHA20_H
#define _CRYPTO_CHACHA20_H

#include <nyx/types.h>

void chacha20_block(const u32 key[8], u32 counter, const u32 nonce[3], u32 out[16]);

#endif
