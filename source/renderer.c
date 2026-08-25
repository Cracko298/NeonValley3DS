#include "renderer.h"
#include <math.h>
#include <stdlib.h>
#include "bloom.h"
#include "config.h"
#include "framebuffer.h"
#include "game.h"
#include "geometry_render.h"
#include "math2d.h"
#include "rng.h"
#include "world.h"

static void world_to_screen(const Game *g,Vec2 p,int *x,int *y) {
    *x=200+(int)((p.x-g->cameraX)*g->cameraScale);
    *y=120+(int)((p.y-g->cameraY)*g->cameraScale);
}

static int wrapi(int v,int m) { v%=m; return v<0?v+m:v; }

static void neon_line(int x0,int y0,int x1,int y1,Color c,bool strong,int bloomMode) {
    if (!fb_clip_line(&x0,&y0,&x1,&y1)) return;
    if (bloomMode>0) {
        bloom_line(x0,y0,x1,y1,c,strong?196:128);
        if (strong) fb_line(x0,y0,x1,y1,1,color_scale(c,0.26f));
    } else if (strong) {
        fb_line(x0,y0,x1,y1,2,color_scale(c,0.13f));
        fb_line(x0,y0,x1,y1,1,color_scale(c,0.34f));
    } else fb_line(x0,y0,x1,y1,1,color_scale(c,0.25f));
    fb_line(x0,y0,x1,y1,0,c);
}

static void particles(const Game *g,Color neon) {
    if (g->settings.particleMode<=0) return;
    for (int layer=0;layer<2;++layer) {
        u32 s=0x4A17BEEFu^(u32)(layer*0x51ED270Bu);
        float factor=layer==0?0.030f:0.075f;
        Color dim=color_scale(neon,layer==0?0.17f:0.29f);
        int count=g->settings.particleMode==1?(layer==0?16:11):(layer==0?34:28);
        int ox=(int)(g->cameraX*factor),oy=(int)(g->cameraY*factor);
        for (int i=0;i<count;++i) {
            int x=wrapi((int)(rng_next(&s)%TOP_W)-ox,TOP_W),y=wrapi((int)(rng_next(&s)%TOP_H)-oy,TOP_H);
            if ((i+layer)%9==0) fb_diamond(x,y,layer+1,dim); else fb_pixel(x,y,dim);
        }
    }
}

static void trail(Game *g,Color neon) {
    int limit=g->settings.trailMode==0?70:(g->settings.trailMode==1?125:TRAIL_POINTS);
    int count=g->trailCount<limit?g->trailCount:limit;
    if (count<2) return;
    int start=(g->trailHead-count+TRAIL_POINTS)%TRAIL_POINTS,px,py;
    world_to_screen(g,g->trail[start],&px,&py);
    for (int j=1;j<count;++j) {
        int idx=(start+j)%TRAIL_POINTS,x,y; world_to_screen(g,g->trail[idx],&x,&y);
        float f=(float)j/(float)count; Color c=color_scale(neon,0.05f+0.95f*f);
        if (g->settings.bloomMode>0&&(j&1)) bloom_line(px,py,x,y,c,92);
        if (g->settings.bloomMode<=0&&j>count-72) { fb_line(px,py,x,y,2,color_scale(c,0.10f)); fb_line(px,py,x,y,1,color_scale(c,0.38f)); }
        else if (j>count-54) fb_line(px,py,x,y,1,color_scale(c,0.22f));
        fb_line(px,py,x,y,0,c); px=x; py=y;
    }
}

static void speed_fx(Game *g,Color neon) {
    if (!g->settings.speedFx) return;
    float ratio=clampf((len2(g->vel)/MAX_SPEED-0.48f)/0.52f,0.0f,1.0f); if (ratio<=0.0f) return;
    float speed=len2(g->vel); Vec2 dir=speed>0.001f?mul2(g->vel,1.0f/speed):v2(1,0);
    int count=3+(int)(ratio*9.0f); u32 r=hash_u32(g->seed^((u32)osGetTime()/33u)); Color c=color_scale(neon,0.10f+ratio*0.22f);
    for (int i=0;i<count;++i) {
        int x=(int)(rng_next(&r)%TOP_W),y=(int)(rng_next(&r)%TOP_H),l=6+(int)(ratio*24.0f)+(int)(rng_next(&r)%10u);
        fb_line(x,y,x-(int)(dir.x*l),y-(int)(dir.y*l),0,c);
    }
}

static void darkness(Game *g) {
    if (!g->darknessActive) return;
    int dx,dy,bx,by; world_to_screen(g,g->darknessPos,&dx,&dy); world_to_screen(g,g->pos,&bx,&by);
    Color red={255,20,28}; int radius=(int)(DARKNESS_RADIUS*g->cameraScale); if (radius<20) radius=20; if (radius>155) radius=155;
    if (dx+radius>=-20&&dx-radius<TOP_W+20&&dy+radius>=-20&&dy-radius<TOP_H+20) {
        bloom_disc(dx,dy,radius+10,red,160); fb_disc(dx,dy,radius+8,color_scale(red,0.08f)); fb_disc(dx,dy,radius+3,color_scale(red,0.18f)); fb_disc(dx,dy,radius,(Color){0,0,0});
        fb_ring(dx,dy,radius,color_scale(red,0.95f)); fb_ring(dx,dy,(int)(radius*0.67f),color_scale(red,0.52f)); fb_ring(dx,dy,(int)(radius*0.38f),color_scale(red,0.82f));
    }
    float danger=g->darknessDanger; if (danger<=0.0f) return;
    int bands=1+(int)(danger*8.0f),vx=dx-bx,vy=dy-by; Color edge=color_scale(red,0.18f+danger*0.55f);
    if (abs(vx)>=abs(vy)) { int x=vx<0?0:TOP_W-1; for (int i=0;i<bands;++i) fb_line(x+(vx<0?i:-i),0,x+(vx<0?i:-i),TOP_H-1,0,color_scale(edge,1.0f-(float)i/(bands+2))); }
    else { int y=vy<0?0:TOP_H-1; for (int i=0;i<bands;++i) fb_line(0,y+(vy<0?i:-i),TOP_W-1,y+(vy<0?i:-i),0,color_scale(edge,1.0f-(float)i/(bands+2))); }
}

static void aim(Game *g,int bx,int by,Color neon) {
    if (!g->aiming) return;
    float a=g->aimAngle; Color dim=color_scale(neon,0.34f); int lx=bx+(int)(cosf(a-1.52f)*19),ly=by+(int)(sinf(a-1.52f)*19);
    for (int i=1;i<=12;++i) { float aa=a-1.52f+1.52f*(float)i/12.0f; int x=bx+(int)(cosf(aa)*19),y=by+(int)(sinf(aa)*19); fb_line(lx,ly,x,y,0,dim); lx=x; ly=y; }
    int x0=bx+(int)(cosf(a)*18),y0=by+(int)(sinf(a)*18),x1=bx+(int)(cosf(a)*31),y1=by+(int)(sinf(a)*31);
    neon_line(x0,y0,x1,y1,neon,true,g->settings.bloomMode);
    float back=a+PI,wing=0.62f; fb_line(x1,y1,x1+(int)(cosf(back-wing)*8),y1+(int)(sinf(back-wing)*8),1,neon); fb_line(x1,y1,x1+(int)(cosf(back+wing)*8),y1+(int)(sinf(back+wing)*8),1,neon);
}

static void ball(Game *g,Color neon) {
    int bx,by; world_to_screen(g,g->pos,&bx,&by); bloom_disc(bx,by,14,neon,220);
    if (g->launchFlash>0.0f) { float t=clampf(g->launchFlash/0.24f,0.0f,1.0f); int r=9+(int)((1.0f-t)*13.0f); bloom_disc(bx,by,r+5,neon,(int)(180.0f*t)); fb_ring(bx,by,r,color_scale(neon,0.28f*t)); }
    fb_disc(bx,by,7,color_scale(neon,0.12f)); fb_ring(bx,by,5,neon); fb_disc(bx,by,3,(Color){244,244,244}); aim(g,bx,by,neon);
}

static void hud(Game *g,Color neon) {
    Color c=color_scale(neon,0.88f); int sw=fb_number_width(g->score,2); fb_number((TOP_W-sw)/2,8,g->score,2,c);
    if (!g->settings.minimalHud) { int speed=(int)len2(g->vel); fb_number((TOP_W-fb_number_width(speed,1))/2,24,speed,1,color_scale(c,0.60f)); }
    fb_number(344-fb_number_width(g->shots,1),205,g->shots,1,color_scale(c,0.82f));
    int shield=(int)(g->shield+0.5f); Color sc=g->darknessDanger>0.55f?(Color){255,40,48}:color_scale(c,0.52f); fb_number(TOP_W-8-fb_number_width(shield,1),10,shield,1,sc);
}

void renderer_boot(void) {
    fb_begin(); fb_clear((Color){0,0,0}); Color c={190,18,255}; bloom_begin(1);
    neon_line(48,176,116,176,c,true,1); neon_line(116,176,146,124,c,true,1); neon_line(146,124,92,84,c,true,1); neon_line(92,84,48,176,c,true,1);
    neon_line(248,64,344,64,color_scale(c,0.8f),true,1); neon_line(344,64,344,154,color_scale(c,0.8f),true,1); neon_line(344,154,248,154,color_scale(c,0.8f),true,1); neon_line(248,154,248,64,color_scale(c,0.8f),true,1);
    fb_disc(205,118,7,color_scale(c,0.12f)); fb_ring(205,118,5,c); fb_disc(205,118,3,(Color){245,245,245}); bloom_apply();
}

void renderer_draw(Game *g) {
    fb_begin(); Color neon=game_neon(g);
    if (g->state==STATE_DEAD) { fb_clear((Color){118,0,8}); int sw=fb_number_width(g->score,2); fb_number((TOP_W-sw)/2,92,g->score,2,(Color){255,210,210}); fb_ring(200,145,18,(Color){255,32,40}); fb_ring(200,145,9,(Color){20,0,0}); return; }
    if (g->impactFlash>0.0f) fb_clear(color_scale(neon,0.055f*clampf(g->impactFlash/0.22f,0.0f,1.0f))); else fb_clear((Color){0,0,0});
    bloom_begin(g->settings.bloomMode); particles(g,neon); speed_fx(g,neon); trail(g,neon); geometry_render(g,neon); darkness(g); ball(g,neon);
    if (g->state==STATE_TITLE) { neon_line(122,75,278,75,neon,true,g->settings.bloomMode); neon_line(122,162,278,162,neon,true,g->settings.bloomMode); fb_triangle_outline(200,118,28,0.0f,neon); }
    bloom_apply(); hud(g,neon);
}
