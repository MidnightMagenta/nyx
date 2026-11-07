// Monocypher version 4.0.3
//
// This file is dual-licensed.  Choose whichever licence you want from
// the two licences listed below.
//
// The first licence is a regular 2-clause BSD licence.  The second licence
// is the CC-0 from Creative Commons. It is intended to release Monocypher
// to the public domain.  The BSD licence serves as a fallback option.
//
// SPDX-License-Identifier: BSD-2-Clause OR CC0-1.0
//
// ------------------------------------------------------------------------
//
// Copyright (c) 2017-2019, Loup Vaillant
// All rights reserved.
//
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the
//    distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// ------------------------------------------------------------------------
//
// Written in 2017-2019 by Loup Vaillant
//
// To the extent possible under law, the author(s) have dedicated all copyright
// and related neighboring rights to this software to the public domain
// worldwide.  This software is distributed without any warranty.
//
// You should have received a copy of the CC0 Public Domain Dedication along
// with this software.  If not, see
// <https://creativecommons.org/publicdomain/zero/1.0/>

#ifndef MONOCYPHER_H
#define MONOCYPHER_H

#include <nyx/stddef.h>
#include <nyx/types.h>

#ifdef MONOCYPHER_CPP_NAMESPACE
namespace MONOCYPHER_CPP_NAMESPACE {
#elif defined(__cplusplus)
extern "C" {
#endif

// Constant time comparisons
// -------------------------

// Return 0 if a and b are equal, -1 otherwise
int crypto_verify16(const u8 a[16], const u8 b[16]);
int crypto_verify32(const u8 a[32], const u8 b[32]);
int crypto_verify64(const u8 a[64], const u8 b[64]);


// Erase sensitive data
// --------------------
void crypto_wipe(void *secret, size_t size);


// Authenticated encryption
// ------------------------
void crypto_aead_lock(u8       *cipher_text,
                      u8        mac[16],
                      const u8  key[32],
                      const u8  nonce[24],
                      const u8 *ad,
                      size_t    ad_size,
                      const u8 *plain_text,
                      size_t    text_size);
int  crypto_aead_unlock(u8       *plain_text,
                        const u8  mac[16],
                        const u8  key[32],
                        const u8  nonce[24],
                        const u8 *ad,
                        size_t    ad_size,
                        const u8 *cipher_text,
                        size_t    text_size);

// Authenticated stream
// --------------------
typedef struct {
    u64 counter;
    u8  key[32];
    u8  nonce[8];
} crypto_aead_ctx;

void crypto_aead_init_x(crypto_aead_ctx *ctx, const u8 key[32], const u8 nonce[24]);
void crypto_aead_init_djb(crypto_aead_ctx *ctx, const u8 key[32], const u8 nonce[8]);
void crypto_aead_init_ietf(crypto_aead_ctx *ctx, const u8 key[32], const u8 nonce[12]);

void crypto_aead_write(crypto_aead_ctx *ctx,
                       u8              *cipher_text,
                       u8               mac[16],
                       const u8        *ad,
                       size_t           ad_size,
                       const u8        *plain_text,
                       size_t           text_size);
int  crypto_aead_read(crypto_aead_ctx *ctx,
                      u8              *plain_text,
                      const u8         mac[16],
                      const u8        *ad,
                      size_t           ad_size,
                      const u8        *cipher_text,
                      size_t           text_size);


// General purpose hash (BLAKE2b)
// ------------------------------

// Direct interface
void crypto_blake2b(u8 *hash, size_t hash_size, const u8 *message, size_t message_size);

void crypto_blake2b_keyed(u8       *hash,
                          size_t    hash_size,
                          const u8 *key,
                          size_t    key_size,
                          const u8 *message,
                          size_t    message_size);

// Incremental interface
typedef struct {
    // Do not rely on the size or contents of this type,
    // for they may change without notice.
    u64    hash[8];
    u64    input_offset[2];
    u64    input[16];
    size_t input_idx;
    size_t hash_size;
} crypto_blake2b_ctx;

void crypto_blake2b_init(crypto_blake2b_ctx *ctx, size_t hash_size);
void crypto_blake2b_keyed_init(crypto_blake2b_ctx *ctx, size_t hash_size, const u8 *key, size_t key_size);
void crypto_blake2b_update(crypto_blake2b_ctx *ctx, const u8 *message, size_t message_size);
void crypto_blake2b_final(crypto_blake2b_ctx *ctx, u8 *hash);


// Password key derivation (Argon2)
// --------------------------------
#define CRYPTO_ARGON2_D  0
#define CRYPTO_ARGON2_I  1
#define CRYPTO_ARGON2_ID 2

typedef struct {
    u32 algorithm; // Argon2d, Argon2i, Argon2id
    u32 nb_blocks; // memory hardness, >= 8 * nb_lanes
    u32 nb_passes; // CPU hardness, >= 1 (>= 3 recommended for Argon2i)
    u32 nb_lanes;  // parallelism level (single threaded anyway)
} crypto_argon2_config;

typedef struct {
    const u8 *pass;
    const u8 *salt;
    u32       pass_size;
    u32       salt_size; // 16 bytes recommended
} crypto_argon2_inputs;

typedef struct {
    const u8 *key;      // may be NULL if no key
    const u8 *ad;       // may be NULL if no additional data
    u32       key_size; // 0 if no key (32 bytes recommended otherwise)
    u32       ad_size;  // 0 if no additional data
} crypto_argon2_extras;

extern const crypto_argon2_extras crypto_argon2_no_extras;

void crypto_argon2(u8                  *hash,
                   u32                  hash_size,
                   void                *work_area,
                   crypto_argon2_config config,
                   crypto_argon2_inputs inputs,
                   crypto_argon2_extras extras);


// Key exchange (X-25519)
// ----------------------

// Shared secrets are not quite random.
// Hash them to derive an actual shared key.
void crypto_x25519_public_key(u8 public_key[32], const u8 secret_key[32]);
void crypto_x25519(u8 raw_shared_secret[32], const u8 your_secret_key[32], const u8 their_public_key[32]);

// Conversion to EdDSA
void crypto_x25519_to_eddsa(u8 eddsa[32], const u8 x25519[32]);

// scalar "division"
// Used for OPRF.  Be aware that exponential blinding is less secure
// than Diffie-Hellman key exchange.
void crypto_x25519_inverse(u8 blind_salt[32], const u8 private_key[32], const u8 curve_point[32]);

// "Dirty" versions of x25519_public_key().
// Use with crypto_elligator_rev().
// Leaks 3 bits of the private key.
void crypto_x25519_dirty_small(u8 pk[32], const u8 sk[32]);
void crypto_x25519_dirty_fast(u8 pk[32], const u8 sk[32]);


// Signatures
// ----------

// EdDSA with curve25519 + BLAKE2b
void crypto_eddsa_key_pair(u8 secret_key[64], u8 public_key[32], u8 seed[32]);
void crypto_eddsa_sign(u8 signature[64], const u8 secret_key[64], const u8 *message, size_t message_size);
int  crypto_eddsa_check(const u8 signature[64], const u8 public_key[32], const u8 *message, size_t message_size);

// Conversion to X25519
void crypto_eddsa_to_x25519(u8 x25519[32], const u8 eddsa[32]);

// EdDSA building blocks
void crypto_eddsa_trim_scalar(u8 out[32], const u8 in[32]);
void crypto_eddsa_reduce(u8 reduced[32], const u8 expanded[64]);
void crypto_eddsa_mul_add(u8 r[32], const u8 a[32], const u8 b[32], const u8 c[32]);
void crypto_eddsa_scalarbase(u8 point[32], const u8 scalar[32]);
int  crypto_eddsa_check_equation(const u8 signature[64], const u8 public_key[32], const u8 h_ram[32]);


// Chacha20
// --------

// Specialised hash.
// Used to hash X25519 shared secrets.
void crypto_chacha20_h(u8 out[32], const u8 key[32], const u8 in[16]);

// Unauthenticated stream cipher.
// Don't forget to add authentication.
u64 crypto_chacha20_djb(u8       *cipher_text,
                        const u8 *plain_text,
                        size_t    text_size,
                        const u8  key[32],
                        const u8  nonce[8],
                        u64       ctr);
u32 crypto_chacha20_ietf(u8       *cipher_text,
                         const u8 *plain_text,
                         size_t    text_size,
                         const u8  key[32],
                         const u8  nonce[12],
                         u32       ctr);
u64 crypto_chacha20_x(u8       *cipher_text,
                      const u8 *plain_text,
                      size_t    text_size,
                      const u8  key[32],
                      const u8  nonce[24],
                      u64       ctr);


// Poly 1305
// ---------

// This is a *one time* authenticator.
// Disclosing the mac reveals the key.
// See crypto_lock() on how to use it properly.

// Direct interface
void crypto_poly1305(u8 mac[16], const u8 *message, size_t message_size, const u8 key[32]);

// Incremental interface
typedef struct {
    // Do not rely on the size or contents of this type,
    // for they may change without notice.
    u8     c[16];  // chunk of the message
    size_t c_idx;  // How many bytes are there in the chunk.
    u32    r[4];   // constant multiplier (from the secret key)
    u32    pad[4]; // random number added at the end (from the secret key)
    u32    h[5];   // accumulated hash
} crypto_poly1305_ctx;

void crypto_poly1305_init(crypto_poly1305_ctx *ctx, const u8 key[32]);
void crypto_poly1305_update(crypto_poly1305_ctx *ctx, const u8 *message, size_t message_size);
void crypto_poly1305_final(crypto_poly1305_ctx *ctx, u8 mac[16]);


// Elligator 2
// -----------

// Elligator mappings proper
void crypto_elligator_map(u8 curve[32], const u8 hidden[32]);
int  crypto_elligator_rev(u8 hidden[32], const u8 curve[32], u8 tweak);

// Easy to use key pair generation
void crypto_elligator_key_pair(u8 hidden[32], u8 secret_key[32], u8 seed[32]);

#ifdef __cplusplus
}
#endif

#endif // MONOCYPHER_H
