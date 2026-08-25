#pragma once

#include "types.h"

bool physics_action_down(u32 held);
void physics_reset(Game *game);
float physics_update(Game *game,float dt,u32 held);
