#include "legacybox.h"
#include "sha256.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

static void set_error(char* out, size_t out_size, const char* message)
{
    if (!out || out_size == 0) return;
    snprintf(out, out_size, "%s", message);
}

static bool ensure_dir(const char* path)
{
    if (mkdir(path, 0777) == 0) return true;
    return errno == EEXIST;
}

bool lbx_load_batch(LbxBatch* out, char* error, size_t error_size)
{
    memset(out, 0, sizeof(*out));

    FILE* f = fopen(LBX_MANIFEST, "rb");
    if (!f) {
        set_error(error, error_size, "batch.tsv not found");
        return false;
    }

    char line[512];
    if (!fgets(line, sizeof(line), f)) {
        fclose(f);
        set_error(error, error_size, "batch.tsv is empty");
        return false;
    }

    char magic[16] = {0};
    int count = 0;
    int start_box = 0;
    if (sscanf(line, "%15[^	]	%64[^	]	%d	%d", magic, out->batch_id, &count, &start_box) != 4 ||
        strcmp(magic, "LBX1") != 0) {
        fclose(f);
        set_error(error, error_size, "invalid LBX1 header");
        return false;
    }

    if (count < 1 || count > LBX_MAX_ENTRIES) {
        fclose(f);
        set_error(error, error_size, "batch count out of range");
        return false;
    }

    out->count = count;
    out->start_box = start_box;

    for (int i = 0; i < count; i++) {
        if (!fgets(line, sizeof(line), f)) {
            fclose(f);
            set_error(error, error_size, "batch.tsv ended early");
            return false;
        }

        LbxEntry* e = &out->entries[i];
        if (sscanf(line, "%95[^	]	%64[^	]	%d	%d",
                   e->filename, e->sha256, &e->box, &e->slot) != 4) {
            fclose(f);
            set_error(error, error_size, "invalid batch row");
            return false;
        }

        if (e->box < 0 || e->slot < 0 || e->slot >= 30) {
            fclose(f);
            set_error(error, error_size, "invalid box/slot");
            return false;
        }
    }

    fclose(f);
    return true;
}

static bool hash_file(const char* path, char out_hex[65])
{
    FILE* f = fopen(path, "rb");
    if (!f) return false;

    LbxSha256 ctx;
    lbx_sha256_init(&ctx);
    uint8_t buf[4096];
    size_t n;

    while ((n = fread(buf, 1, sizeof(buf), f)) > 0)
        lbx_sha256_update(&ctx, buf, n);

    bool ok = !ferror(f);
    fclose(f);
    if (!ok) return false;

    uint8_t digest[32];
    lbx_sha256_final(&ctx, digest);
    lbx_sha256_hex(digest, out_hex);
    return true;
}

bool lbx_verify_batch(const LbxBatch* batch, int* verified, char* error, size_t error_size)
{
    *verified = 0;

    for (int i = 0; i < batch->count; i++) {
        char path[256];
        snprintf(path, sizeof(path), "%s/%s", LBX_INBOX, batch->entries[i].filename);

        FILE* f = fopen(path, "rb");
        if (!f) {
            snprintf(error, error_size, "missing: %s", batch->entries[i].filename);
            return false;
        }
        fseek(f, 0, SEEK_END);
        long size = ftell(f);
        fclose(f);

        // PK7 stored and party-format files are both accepted by the PKSM adapter.
        if (size != 232 && size != 260) {
            snprintf(error, error_size, "bad PK7 size: %s", batch->entries[i].filename);
            return false;
        }

        char actual[65];
        if (!hash_file(path, actual)) {
            snprintf(error, error_size, "hash read failed: %s", batch->entries[i].filename);
            return false;
        }
        if (strcmp(actual, batch->entries[i].sha256) != 0) {
            snprintf(error, error_size, "SHA256 mismatch: %s", batch->entries[i].filename);
            return false;
        }
        (*verified)++;
    }

    return true;
}

bool lbx_write_ready_flag(const LbxBatch* batch, char* error, size_t error_size)
{
    if (!ensure_dir(LBX_ROOT)) {
        set_error(error, error_size, "cannot create LegacyBox directory");
        return false;
    }

    FILE* f = fopen(LBX_READY_FLAG ".tmp", "wb");
    if (!f) {
        set_error(error, error_size, "cannot write ready.flag");
        return false;
    }

    bool ok = fprintf(f, "LBX1\n%s\n%d\n", batch->batch_id, batch->count) > 0;
    if (fclose(f) != 0) ok = false;
    if (ok && rename(LBX_READY_FLAG ".tmp", LBX_READY_FLAG) == 0) return true;
    remove(LBX_READY_FLAG ".tmp");
    set_error(error, error_size, "ready.flag write/rename failed");
    return false;
}

