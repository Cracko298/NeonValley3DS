#pragma once

#include <3ds.h>

u32 rng_next(u32 *state);
float rng_unit(u32 *state);
float rng_range(u32 *state,float lo,float hi);
int rng_chance(u32 *state,int pct);
u32 hash_u32(u32 x);
u32 rng_run_seed(u32 previous);
