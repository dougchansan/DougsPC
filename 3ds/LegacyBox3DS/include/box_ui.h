#pragma once
#include "frontend.h"
typedef struct {
    const LbxFrontend* state;
    const int* visible;
    int visible_count, tab, option, preset, checkpoint, seed_digit;
    bool loaded, verified;
    int batch_count;
    const char* message;
} LbxView;
void lbx_ui_init(void);
void lbx_ui_exit(void);
void lbx_ui_render(const LbxView* view);
/* Pure geometry: -1 outside a populated cell, otherwise visible index. */
int lbx_grid_hit(int x, int y, int first, int count);
int lbx_grid_move(int cursor, int count, int dx, int dy);
