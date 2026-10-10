#pragma once
#include "local_rng.h"
#include <stdbool.h>
#include <limits.h>
typedef struct { int dex, ivs[6], hidden_power; const char* hash; } LbxSearchProfile;
typedef struct {
    uint64_t state;
    const LbxSearchProfile* profile;
    LbxLocalRoll best;
    int attempts, budget, best_attempt, score;
    bool done, exact;
} LbxSearch;
/* HP, Atk, Def, Spe, SpA, SpD: this order is also the Hidden Power bit order. */
static inline int lbx_hidden_power(uint32_t packed) {
    unsigned bits=0;
    for(unsigned i=0;i<6;i++) bits|=((packed>>(5*i))&1u)<<i;
    return (int)(bits*15/63);
}
static inline int lbx_score(const LbxSearchProfile* p, uint32_t packed, bool* exact) {
    bool match=p->hidden_power<0 || lbx_hidden_power(packed)==p->hidden_power;
    int score=match?10000:0;
    *exact=match;
    for(unsigned i=0;i<6;i++) if(p->ivs[i]>=0) {
        int got=(int)((packed>>(i*5))&31), diff=got-p->ivs[i];
        if(diff==0) score+=100;
        else {score-=diff<0?-diff:diff;*exact=false;}
    }
    return score;
}
static inline void lbx_search_init(LbxSearch* s,uint32_t seed,const LbxSearchProfile* p,int budget) {
    *s=(LbxSearch){.state=((uint64_t)seed<<32)|(unsigned)p->dex,
                   .profile=p,.budget=budget,.score=INT_MIN};
}
/* Bounded chunks let the frontend poll cancellation and draw progress. */
static inline void lbx_search_step(LbxSearch* s,int work) {
    while(work-->0 && !s->done) {
        LbxLocalRoll candidate=lbx_local_roll_next(&s->state,(unsigned)s->profile->dex);
        bool exact=false;int score=lbx_score(s->profile,candidate.ivs,&exact);
        s->attempts++;
        if(score>s->score) {
            s->best=candidate;s->score=score;s->best_attempt=s->attempts;s->exact=exact;
        }
        s->done=exact || s->attempts>=s->budget;
    }
}
