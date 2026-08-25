#include "rng.h"

u32 rng_next(u32 *state) {
    u32 x=*state;
    if (!x) x=0xA341316Cu;
    x^=x<<13;
    x^=x>>17;
    x^=x<<5;
    *state=x;
    return x;
}

float rng_unit(u32 *state) {
    return (rng_next(state)&0x00FFFFFFu)/16777215.0f;
}

float rng_range(u32 *state,float lo,float hi) {
    return lo+(hi-lo)*rng_unit(state);
}

int rng_chance(u32 *state,int pct) {
    return (int)(rng_next(state)%100u)<pct;
}

u32 hash_u32(u32 x) {
    x^=x>>16;
    x*=0x7feb352du;
    x^=x>>15;
    x*=0x846ca68bu;
    x^=x>>16;
    return x;
}

u32 rng_run_seed(u32 previous) {
    static u32 counter=0x6D2B79F5u;
    u64 t=osGetTime();
    counter=hash_u32(counter+0x9E3779B9u);
    u32 s=(u32)t^(u32)(t>>32)^previous^counter;
    s=hash_u32(s);
    return s?s:0xA341316Cu;
}
