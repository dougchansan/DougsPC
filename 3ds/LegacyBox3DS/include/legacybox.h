#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define LBX_ROOT "/3ds/LegacyBox"
#define LBX_INBOX LBX_ROOT "/inbox/current"
#define LBX_MANIFEST LBX_INBOX "/batch.tsv"
#define LBX_READY_FLAG LBX_ROOT "/ready.flag"
#define LBX_MAX_ENTRIES 900
#define LBX_FILENAME_MAX 96
#define LBX_SHA256_HEX 64

typedef struct {
    char filename[LBX_FILENAME_MAX];
    char sha256[LBX_SHA256_HEX + 1];
    int box;
    int slot;
} LbxEntry;

typedef struct {
    char batch_id[65];
    int count;
    int start_box;
    LbxEntry entries[LBX_MAX_ENTRIES];
} LbxBatch;

bool lbx_load_batch(LbxBatch* out, char* error, size_t error_size);
bool lbx_verify_batch(const LbxBatch* batch, int* verified, char* error, size_t error_size);
bool lbx_write_ready_flag(const LbxBatch* batch, char* error, size_t error_size);
