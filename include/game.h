#pragma once

#include "types.h"

void game_init(Game *game);
void game_new_run(Game *game);
void game_update(Game *game,float dt,u32 held);
Color game_neon(const Game *game);
