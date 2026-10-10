#pragma once
#include "frontend.h"
#include <stdint.h>
#define LBX_MOON_SAVE_SIZE 0x6BE00u
#ifdef __cplusplus
extern "C" {
#endif
/* Each callback opens/closes the same pinned game archive; write commits it. */
typedef struct {
    bool (*read)(uint8_t*, size_t, char*, size_t, void*);
    bool (*write)(const uint8_t*, size_t, char*, size_t, void*);
    void* context;
    const char* label;
} LbxSaveIO;
/* Prepare edits in memory. Neither call writes the game until commit. */
bool lbx_moon_prepare(const LbxFrontend*, const LbxSaveIO*, LbxSearchProgress, void*, char*, size_t);
bool lbx_moon_prepare_batch(const LbxSaveIO*, char*, size_t);
bool lbx_moon_commit(char*, size_t);
void lbx_moon_discard(void);
bool lbx_moon_pending(void);
/* 3DS adapter: Moon cartridge preferred, otherwise the installed Moon title. */
bool lbx_moon_archive(LbxSaveIO*, char*, size_t);
#ifdef __cplusplus
}
#endif
