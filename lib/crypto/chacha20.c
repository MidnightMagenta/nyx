#include <crypto/crypto.h>
#include <nyx/types.h>

#define ROTL(x, n) (((x) << (n)) | ((x) >> (32 - (n))))
// clang-format off
#define QR(a, b, c, d) do {                 \
    a += b; d ^= a; d = ROTL(d, 16);        \
    c += d; b ^= c; b = ROTL(b, 12);        \
    a += b; d ^= a; d = ROTL(d, 8);         \
    c += d; b ^= c; b = ROTL(b, 7);         \
} while (0)
// clang-format on

void chacha20_block(const u32 key[8], u32 counter, const u32 nonce[3], u32 out[16]) {
    u32 in[16] = {
            0x61707865,
            0x3320646e,
            0x79622d32,
            0x6b206574,
            key[0],
            key[1],
            key[2],
            key[3],
            key[4],
            key[5],
            key[6],
            key[7],
            counter,
            nonce[0],
            nonce[1],
            nonce[2],
    };
    u32 x[16];
    int i;

    for (i = 0; i < 16; i++) { x[i] = in[i]; }
    for (i = 0; i < 10; i++) {
        QR(x[0], x[4], x[8], x[12]);
        QR(x[1], x[5], x[9], x[13]);
        QR(x[2], x[6], x[10], x[14]);
        QR(x[3], x[7], x[11], x[15]);
        QR(x[0], x[5], x[10], x[15]);
        QR(x[1], x[6], x[11], x[12]);
        QR(x[2], x[7], x[8], x[13]);
        QR(x[3], x[4], x[9], x[14]);
    }
    for (i = 0; i < 16; i++) { out[i] = x[i] + in[i]; }
}
