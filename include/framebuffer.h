#pragma once

#include "types.h"

void fb_begin(void);
void fb_clear(Color c);
void fb_pixel(int x,int y,Color c);
void fb_add_pixel(int x,int y,Color c);
void fb_disc(int cx,int cy,int r,Color c);
void fb_ring(int cx,int cy,int r,Color c);
void fb_line(int x0,int y0,int x1,int y1,int radius,Color c);
bool fb_clip_line(int *x0,int *y0,int *x1,int *y1);
Color color_scale(Color c,float scale);
void fb_diamond(int cx,int cy,int r,Color c);
void fb_number(int x,int y,int n,int scale,Color c);
int fb_number_width(int n,int scale);
void fb_triangle_outline(int cx,int cy,int r,float angle,Color c);
void fb_polygon_fill(const int *x,const int *y,int count,Color c);
