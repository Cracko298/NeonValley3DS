#pragma once

#include <3ds.h>
#include <stdbool.h>

bool audio_init(void);
void audio_exit(void);
void audio_update(void);
void audio_set_volume(int level);
int audio_get_volume(void);
bool audio_is_ready(void);
