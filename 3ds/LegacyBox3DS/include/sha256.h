#pragma once
#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint32_t state[8];
    uint64_t bitlen;
    uint8_t data[64];
    size_t datalen;
} LbxSha256;

void lbx_sha256_init(LbxSha256* ctx);
void lbx_sha256_update(LbxSha256* ctx, const uint8_t* data, size_t len);
void lbx_sha256_final(LbxSha256* ctx, uint8_t out[32]);
void lbx_sha256_hex(const uint8_t digest[32], char out[65]);
