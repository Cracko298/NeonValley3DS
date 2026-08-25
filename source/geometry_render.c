#include "geometry_render.h"
#include <math.h>
#include "bloom.h"
#include "config.h"
#include "framebuffer.h"
#include "world.h"

static void to_screen(const Game *g,Vec2 p,int *x,int *y) {
    *x=200+(int)((p.x-g->cameraX)*g->cameraScale);
    *y=120+(int)((p.y-g->cameraY)*g->cameraScale);
}

static Color accent_color(Color base,int accent) {
    if (!accent) return base;
    Color c={base.g,base.b,base.r};
    return c;
}

static bool same_point(Vec2 a,Vec2 b) {
    return fabsf(a.x-b.x)<0.01f&&fabsf(a.y-b.y)<0.01f;
}

static void shape_line(int x0,int y0,int x1,int y1,Color c,int bloomMode) {
    if (!fb_clip_line(&x0,&y0,&x1,&y1)) return;
    if (bloomMode>0) bloom_line(x0,y0,x1,y1,c,220);
    fb_line(x0,y0,x1,y1,3,color_scale(c,bloomMode>0?0.10f:0.08f));
    fb_line(x0,y0,x1,y1,2,color_scale(c,bloomMode>0?0.22f:0.18f));
    fb_line(x0,y0,x1,y1,1,color_scale(c,0.58f));
    fb_line(x0,y0,x1,y1,0,c);
}

static void fill_sector(const Game *g,const Sector *sec) {
    int i=0;
    while (i<sec->segCount) {
        Vec2 first=sec->seg[i].a;
        u32 gid=sec->seg[i].geometryId;
        int x[16],y[16],count=0;
        bool closed=false;
        while (i<sec->segCount&&sec->seg[i].geometryId==gid&&count<16) {
            const Segment *s=&sec->seg[i];
            to_screen(g,s->a,&x[count],&y[count]);
            ++count;
            ++i;
            if (same_point(s->b,first)) { closed=true; break; }
        }
        if (closed&&count>=3) fb_polygon_fill(x,y,count,(Color){0,0,0});
        if (!closed) {
            while (i<sec->segCount&&sec->seg[i].geometryId==gid) ++i;
        }
    }
}

void geometry_render(Game *g,Color neon) {
    float halfW=(TOP_W*0.5f)/g->cameraScale+WORLD_RENDER_PAD;
    float halfH=(TOP_H*0.5f)/g->cameraScale+WORLD_RENDER_PAD;
    int minX=(int)floorf((g->cameraX-halfW)/SECTOR_W),maxX=(int)floorf((g->cameraX+halfW)/SECTOR_W);
    int minY=(int)floorf((g->cameraY-halfH)/SECTOR_H),maxY=(int)floorf((g->cameraY+halfH)/SECTOR_H);

    for (int sy=minY;sy<=maxY;++sy) for (int sx=minX;sx<=maxX;++sx)
        fill_sector(g,world_get_sector(&g->world,sx,sy));

    for (int sy=minY;sy<=maxY;++sy) for (int sx=minX;sx<=maxX;++sx) {
        Sector *sec=world_get_sector(&g->world,sx,sy);
        for (int i=0;i<sec->segCount;++i) {
            int x0,y0,x1,y1;
            to_screen(g,sec->seg[i].a,&x0,&y0);
            to_screen(g,sec->seg[i].b,&x1,&y1);
            shape_line(x0,y0,x1,y1,accent_color(neon,sec->seg[i].accent),g->settings.bloomMode);
        }
    }
}
