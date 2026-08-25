#pragma once

#include "types.h"

void world_cache_reset(void);
Sector *world_get_sector(const World *world,int sx,int sy);
u32 world_sector_seed(u32 seed,int sx,int sy);
