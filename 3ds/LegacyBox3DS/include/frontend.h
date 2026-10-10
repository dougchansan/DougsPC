#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#define LBX_DEX_COUNT 807
#ifndef LBX_ROOT
#define LBX_ROOT "/3ds/LegacyBox"
#endif
#define LBX_REQUEST LBX_ROOT "/request.tsv"
typedef struct {
    bool selected[LBX_DEX_COUNT];
    int cursor, generation, level, nature, shiny, start_box;
    int rng_mode, rng_budget;
    uint32_t rng_seed;
} LbxFrontend;
extern const char* const lbx_species[LBX_DEX_COUNT];
extern const unsigned char lbx_generations[LBX_DEX_COUNT];
#ifdef __cplusplus
extern "C" {
#endif
void lbx_frontend_init(LbxFrontend* state);
int lbx_selected_count(const LbxFrontend* state);
void lbx_preset(LbxFrontend* state, int preset);
bool lbx_save_request(const LbxFrontend* state, const char* path, char* error, size_t size);


/* Return false to cancel; partial exports never replace the previous request. */
typedef bool (*LbxSearchProgress)(int dex,int completed,int total,int attempts,int budget,int score,void* context);
bool lbx_save_request_progress(const LbxFrontend* state,const char* path,char* error,size_t size,
                               LbxSearchProgress progress,void* context);

#ifdef __cplusplus
}
#endif
