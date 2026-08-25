#pragma once

#include "types.h"

void bloom_begin(int mode);
void bloom_line(int x0,int y0,int x1,int y1,Color c,int strength);
void bloom_disc(int cx,int cy,int radius,Color c,int strength);
void bloom_apply(void);
