"""Exercise cell hitboxes, navigation and framebuffer bounds with host GCC."""
import os
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]

class BoxGridTests(unittest.TestCase):
    def test_grid_and_renderer(self):
        with tempfile.TemporaryDirectory() as td:
            folder = Path(td)
            (folder / '3ds.h').write_text('''#pragma once
#include <stdint.h>
typedef enum {GFX_TOP,GFX_BOTTOM} gfxScreen_t;
typedef enum {GFX_LEFT} gfx3dSide_t;
uint8_t* gfxGetFramebuffer(gfxScreen_t,gfx3dSide_t,uint16_t*,uint16_t*);
''')
            (folder / 'probe.c').write_text('''#include <assert.h>
#include <stdio.h>
#include <3ds.h>
#include "box_ui.h"
static unsigned char top[400*240*3],bottom[320*240*3];
uint8_t* gfxGetFramebuffer(gfxScreen_t s,gfx3dSide_t side,uint16_t* w,uint16_t* h) {
    (void)side;(void)w;(void)h;return s==GFX_TOP?top:bottom;
}
int main(int argc,char** argv) {
    assert(lbx_grid_hit(10,48,0,807)==0);
    assert(lbx_grid_hit(309,197,0,807)==-1);
    assert(lbx_grid_hit(307,195,0,807)==29);
    assert(lbx_grid_hit(10,48,780,807)==780);
    assert(lbx_grid_hit(160,168,780,807)==-1);
    assert(lbx_grid_hit(9,48,0,807)==-1);
    assert(lbx_grid_move(5,807,0,1)==11);
    assert(lbx_grid_move(0,807,-1,0)==0);
    assert(lbx_grid_move(806,807,0,1)==806);
    LbxFrontend state;lbx_frontend_init(&state);
    int visible[807];for(int i=0;i<807;i++)visible[i]=i;
    LbxView v={.state=&state,.visible=visible,.visible_count=807,
        .message="Choose Pokemon, then save a request."};
    lbx_ui_init();
    for(int tab=0;tab<5;tab++) {v.tab=tab;lbx_ui_render(&v);}
    state.cursor=806;state.selected[806]=1;v.tab=0;lbx_ui_render(&v);
    state.cursor=0;state.selected[0]=1;lbx_ui_render(&v);
    if(argc>1) {FILE* f=fopen(argv[1],"wb");assert(f);fwrite(top,1,sizeof(top),f);fwrite(bottom,1,sizeof(bottom),f);fclose(f);}
    lbx_ui_exit();return 0;
}
''')
            subprocess.run(['gcc','-std=c11','-Wall','-Wextra','-Werror',
                '-fsanitize=address,undefined','-I'+td,
                '-I'+str(ROOT/'3ds/LegacyBox3DS/include'),
                str(folder/'probe.c'),
                *[str(ROOT/'3ds/LegacyBox3DS/source'/n) for n in
                  ('box_grid.c','box_ui.c','frontend.c')],
                '-o',str(folder/'probe')], check=True)
            env = dict(os.environ, ASAN_OPTIONS='detect_leaks=0')
            subprocess.run([str(folder/'probe')], check=True, env=env)

if __name__ == '__main__': unittest.main()
