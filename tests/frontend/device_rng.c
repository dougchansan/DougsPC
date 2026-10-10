#include "frontend.h"
#include "local_rng.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
static bool cancel_search(int dex,int completed,int total,int attempts,int budget,int score,void* context) {
    (void)dex;(void)completed;(void)total;(void)budget;(void)score;(void)context;
    return attempts<256;
}
int main(int argc, char** argv) {
    assert(argc==2);
    for(unsigned d=1;d<=151;d++) {
        LbxLocalRoll a=lbx_local_roll(0xFFFFFFFFu,d), b=lbx_local_roll(0xFFFFFFFFu,d);
        assert(a.ec==b.ec && a.pid==b.pid && a.ivs==b.ivs);
        assert(a.ivs>>30==0);
        unsigned perfect=0;
        for(unsigned i=0;i<6;i++) perfect+=((a.ivs>>(i*5))&31)==31;
        assert(perfect>=(d==151?5u:3u));
    }
    LbxFrontend s;char error[160];lbx_frontend_init(&s);lbx_preset(&s,0);
    s.rng_mode=2;s.rng_seed=0xFFFFFFFFu;
    assert(lbx_save_request(&s,argv[1],error,sizeof(error)));
    FILE* f=fopen(argv[1],"rb");assert(f);char before[4096],after[4096];
    size_t n=fread(before,1,sizeof(before),f);fclose(f);
    assert(!lbx_save_request_progress(&s,argv[1],error,sizeof(error),cancel_search,NULL));
    f=fopen(argv[1],"rb");assert(f);assert(fread(after,1,sizeof(after),f)==n);fclose(f);
    for(size_t i=0;i<n;i++)assert(before[i]==after[i]);
    s.selected[9]=true;
    assert(!lbx_save_request(&s,argv[1],error,sizeof(error)));
    s.selected[151]=true;
    assert(!lbx_save_request(&s,argv[1],error,sizeof(error)));
    puts("Device RNG export PASS");return 0;
}
