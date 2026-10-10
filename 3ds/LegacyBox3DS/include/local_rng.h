#pragma once
#include <stdint.h>
/* LBX-DEVICE-ROLLS-1. Explicit unsigned arithmetic is portable to ARM/host. */
typedef struct { uint32_t ec, pid, ivs; } LbxLocalRoll;
static inline uint64_t lbx_rng_next(uint64_t* s) {
    uint64_t z=(*s+=UINT64_C(0x9E3779B97F4A7C15));
    z=(z^(z>>30))*UINT64_C(0xBF58476D1CE4E5B9);
    z=(z^(z>>27))*UINT64_C(0x94D049BB133111EB);
    return z^(z>>31);
}
static inline LbxLocalRoll lbx_local_roll(uint32_t seed, unsigned dex) {
    uint64_t state=((uint64_t)seed<<32)|dex;
    LbxLocalRoll roll;
    roll.ec=(uint32_t)lbx_rng_next(&state);
    roll.pid=(uint32_t)lbx_rng_next(&state);
    roll.ivs=0;
    unsigned values[6], perfect[6]={0};
    for(unsigned i=0;i<6;i++) values[i]=(unsigned)(lbx_rng_next(&state)&31);
    /* VC guarantees three perfect IVs (five for Mew/Celebi). */
    unsigned required=dex==151?5:3;
    for(unsigned n=0;n<required;) {
        unsigned i=(unsigned)(lbx_rng_next(&state)%6);
        if(!perfect[i]) {perfect[i]=1;values[i]=31;n++;}
    }
    for(unsigned i=0;i<6;i++) roll.ivs|=(uint32_t)values[i]<<(5*i);
    return roll;
}
