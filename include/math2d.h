#pragma once

#include <math.h>
#include "types.h"

static inline float clampf(float v,float lo,float hi) { return v<lo?lo:(v>hi?hi:v); }
static inline Vec2 v2(float x,float y) { Vec2 v={x,y}; return v; }
static inline Vec2 add2(Vec2 a,Vec2 b) { return v2(a.x+b.x,a.y+b.y); }
static inline Vec2 sub2(Vec2 a,Vec2 b) { return v2(a.x-b.x,a.y-b.y); }
static inline Vec2 mul2(Vec2 a,float s) { return v2(a.x*s,a.y*s); }
static inline float dot2(Vec2 a,Vec2 b) { return a.x*b.x+a.y*b.y; }
static inline float len2sq(Vec2 a) { return dot2(a,a); }
static inline float len2(Vec2 a) { return sqrtf(len2sq(a)); }

static inline float wrap_angle(float a) {
    while (a>PI) a-=2.0f*PI;
    while (a<-PI) a+=2.0f*PI;
    return a;
}

static inline Vec2 rotate2(Vec2 p,float a) {
    float c=cosf(a),s=sinf(a);
    return v2(p.x*c-p.y*s,p.x*s+p.y*c);
}

static inline Vec2 closest_on_segment(Vec2 p,Vec2 a,Vec2 b) {
    Vec2 ab=sub2(b,a);
    float d=len2sq(ab);
    if (d<0.00001f) return a;
    float t=clampf(dot2(sub2(p,a),ab)/d,0.0f,1.0f);
    return add2(a,mul2(ab,t));
}
