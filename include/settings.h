#pragma once

#include "types.h"

void settings_defaults(Settings *settings);
void settings_adjust(Game *game,int dir);
const char *settings_bloom_name(int v);
const char *settings_zoom_name(int v);
const char *settings_particle_name(int v);
const char *settings_trail_name(int v);
const char *settings_music_name(int v);
