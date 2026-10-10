#include "search_rng.h"
#include <assert.h>
#include <stdio.h>
static uint32_t pack(const int* values) {
    uint32_t packed=0;for(unsigned i=0;i<6;i++)packed|=(unsigned)values[i]<<(i*5);return packed;
}
int main(void) {
    LbxSearchProfile fire={1,{31,0,31,30,30,31},8,""};
    bool exact=false;
    assert(lbx_hidden_power(0)==0 && lbx_hidden_power(0x3FFFFFFF)==15);
    assert(lbx_hidden_power(pack(fire.ivs))==8);
    assert(lbx_score(&fire,pack(fire.ivs),&exact)==10600 && exact);
    LbxSearchProfile impossible={1,{0,0,0,0,0,0},-1,""};
    int oldscore=-2147483647;
    for(int n=0;n<3;n++) {
        int budget=(int[]){5000,50000,250000}[n];
        LbxSearch a,b;lbx_search_init(&a,0xFFFFFFFF,&impossible,budget);
        lbx_search_init(&b,0xFFFFFFFF,&impossible,budget);
        while(!a.done)lbx_search_step(&a,256);
        while(!b.done)lbx_search_step(&b,73);
        assert(a.attempts==budget && !a.exact && a.score>=oldscore);
        assert(a.best.ec==b.best.ec && a.best.pid==b.best.pid && a.best.ivs==b.best.ivs);
        assert(a.best_attempt==b.best_attempt && a.score==b.score);
        /* Independently enumerate all candidates and scores to verify best/tie handling. */
        uint64_t state=((uint64_t)0xFFFFFFFF<<32)|1u;
        int best=-2147483647, best_attempt=0;
        for(int i=1;i<=budget;i++) {
            LbxLocalRoll roll=lbx_local_roll_next(&state,1);
            int score=10000;
            for(unsigned j=0;j<6;j++) {int iv=(int)((roll.ivs>>(j*5))&31);score+=iv==0?100:-iv;}
            if(score>best) {best=score;best_attempt=i;}
        }
        assert(a.score==best && a.best_attempt==best_attempt);
        oldscore=a.score;
    }
    LbxSearchProfile any={1,{-1,-1,-1,-1,-1,-1},-1,""};
    LbxSearch early;lbx_search_init(&early,1,&any,250000);lbx_search_step(&early,256);
    assert(early.done && early.exact && early.attempts==1);
    puts("Native IV search/scoring PASS");return 0;
}
