#include "world.h"
#include <math.h>
#include <string.h>
#include "math2d.h"
#include "rng.h"

typedef struct {
    bool valid;
    int sx, sy;
    u32 stamp;
    Sector sector;
} SectorCacheEntry;

typedef enum {
    SIZE_SMALL=0,
    SIZE_MEDIUM,
    SIZE_LARGE,
    SIZE_GIGANTIC,
    SIZE_BEHEMOTH
} ShapeSize;

typedef struct {
    int sx, sy, index;
    ShapeSize tier;
    Vec2 center;
    float width, height;
    float angle;
    float minx, maxx, miny, maxy;
    int shape;
    u8 kind;
    u8 accent;
    u32 geometryId;
    u32 priority;
    bool starter;
} ShapeCandidate;

#define MAX_SHAPES_PER_SECTOR 3
#define SHAPE_GAP 140.0f
#define OVERLAP_SCAN_X 5
#define OVERLAP_SCAN_Y 7

#define CANDIDATE_CACHE_SLOTS 512

typedef struct {
    bool valid;
    u32 seed;
    int sx, sy;
    int count;
    ShapeCandidate candidate[MAX_SHAPES_PER_SECTOR];
} CandidateCacheEntry;

static SectorCacheEntry cache[SECTOR_CACHE_SLOTS];
static CandidateCacheEntry candidateCache[CANDIDATE_CACHE_SLOTS];
static u32 cacheStamp;

u32 world_sector_seed(u32 seed,int sx,int sy) {
    u32 x=(u32)sx;
    u32 y=(u32)sy;
    return hash_u32(seed^hash_u32(x*0x9E3779B9u)^hash_u32(y*0x85EBCA6Bu));
}

static u32 geometry_id(const World *w,int sx,int sy,u32 salt) {
    u32 id=hash_u32(world_sector_seed(w->seed,sx,sy)^salt);
    return id?id:1u;
}

static void add_segment(Sector *s,Vec2 a,Vec2 b,u8 kind,u8 accent,u32 gid) {
    if (s->segCount>=MAX_SEG_PER_SECTOR) return;
    Segment *o=&s->seg[s->segCount++];
    o->a=a;
    o->b=b;
    o->minx=fminf(a.x,b.x)-COLLISION_MARGIN;
    o->maxx=fmaxf(a.x,b.x)+COLLISION_MARGIN;
    o->miny=fminf(a.y,b.y)-COLLISION_MARGIN;
    o->maxy=fmaxf(a.y,b.y)+COLLISION_MARGIN;
    o->kind=kind;
    o->accent=accent;
    o->geometryId=gid;
}

static void add_closed(Sector *s,const Vec2 *p,int count,u8 kind,u8 accent,u32 gid) {
    for (int i=0;i<count;++i) add_segment(s,p[i],p[(i+1)%count],kind,accent,gid);
}

static void add_regular(Sector *s,Vec2 c,float rx,float ry,int sides,float angle,u8 kind,u8 accent,u32 gid) {
    Vec2 p[12];
    if (sides>12) sides=12;
    for (int i=0;i<sides;++i) {
        float a=angle+2.0f*PI*(float)i/(float)sides;
        p[i]=v2(c.x+cosf(a)*rx,c.y+sinf(a)*ry);
    }
    add_closed(s,p,sides,kind,accent,gid);
}

static void add_rectangle(Sector *s,Vec2 c,float w,float h,float angle,u8 kind,u8 accent,u32 gid) {
    Vec2 local[4]={v2(-w*0.5f,-h*0.5f),v2(w*0.5f,-h*0.5f),v2(w*0.5f,h*0.5f),v2(-w*0.5f,h*0.5f)};
    Vec2 p[4];
    for (int i=0;i<4;++i) p[i]=add2(c,rotate2(local[i],angle));
    add_closed(s,p,4,kind,accent,gid);
}

static void add_trapezoid(Sector *s,Vec2 c,float w,float h,float angle,u8 kind,u8 accent,u32 gid) {
    float top=w*0.58f;
    Vec2 local[4]={v2(-w*0.5f,h*0.5f),v2(w*0.5f,h*0.5f),v2(top*0.5f,-h*0.5f),v2(-top*0.5f,-h*0.5f)};
    Vec2 p[4];
    for (int i=0;i<4;++i) p[i]=add2(c,rotate2(local[i],angle));
    add_closed(s,p,4,kind,accent,gid);
}

static void add_star(Sector *s,Vec2 c,float rx,float ry,float angle,u8 kind,u8 accent,u32 gid) {
    Vec2 p[10];
    for (int i=0;i<10;++i) {
        float a=angle+2.0f*PI*(float)i/10.0f;
        float k=(i&1)?0.46f:1.0f;
        p[i]=v2(c.x+cosf(a)*rx*k,c.y+sinf(a)*ry*k);
    }
    add_closed(s,p,10,kind,accent,gid);
}

static ShapeSize choose_size(u32 *r) {
    int roll=(int)(rng_next(r)%100u);
    if (roll<32) return SIZE_SMALL;
    if (roll<70) return SIZE_MEDIUM;
    if (roll<90) return SIZE_LARGE;
    if (roll<98) return SIZE_GIGANTIC;
    return SIZE_BEHEMOTH;
}

static void size_dims(u32 *r,ShapeSize tier,float *w,float *h) {
    float width;
    if (tier==SIZE_SMALL) width=rng_range(r,180.0f,480.0f);
    else if (tier==SIZE_MEDIUM) width=rng_range(r,560.0f,1150.0f);
    else if (tier==SIZE_LARGE) width=rng_range(r,1250.0f,2200.0f);
    else if (tier==SIZE_GIGANTIC) width=rng_range(r,2400.0f,3800.0f);
    else width=rng_range(r,4300.0f,6200.0f);
    *w=width;
    *h=width*rng_range(r,tier>=SIZE_GIGANTIC?0.55f:0.62f,tier>=SIZE_GIGANTIC?0.84f:1.05f);
}

static void candidate_bounds(ShapeCandidate *c) {
    float hw=c->width*0.5f;
    float hh=c->height*0.5f;
    float ex=hw;
    float ey=hh;
    if (c->shape==1||c->shape==2||c->shape==7) {
        if (c->shape==1) hh=hw;
        float ca=fabsf(cosf(c->angle));
        float sa=fabsf(sinf(c->angle));
        ex=ca*hw+sa*hh;
        ey=sa*hw+ca*hh;
    }
    c->minx=c->center.x-ex;
    c->maxx=c->center.x+ex;
    c->miny=c->center.y-ey;
    c->maxy=c->center.y+ey;
}

static int build_candidates(const World *w,int sx,int sy,ShapeCandidate out[MAX_SHAPES_PER_SECTOR]) {
    u32 r=world_sector_seed(w->seed,sx,sy);
    if ((sx!=0||sy!=0)&&!rng_chance(&r,76)) return 0;

    float ox=(float)sx*SECTOR_W;
    float oy=(float)sy*SECTOR_H;
    ShapeSize first=choose_size(&r);
    int count=1;
    if (first<=SIZE_MEDIUM) count=rng_chance(&r,48)?3:2;
    else if (first==SIZE_LARGE) count=rng_chance(&r,42)?2:1;

    for (int i=0;i<count;++i) {
        ShapeCandidate *c=&out[i];
        memset(c,0,sizeof(*c));
        c->sx=sx;
        c->sy=sy;
        c->index=i;
        c->tier=i==0?first:choose_size(&r);
        if (i>0&&c->tier>SIZE_LARGE) c->tier=SIZE_MEDIUM;
        if (sx==0&&sy==0&&i==0) {
            c->tier=SIZE_MEDIUM;
            c->center=v2(ox+SECTOR_W*0.78f,oy+SECTOR_H*0.70f);
            c->starter=true;
        } else {
            c->center=v2(ox+rng_range(&r,120.0f,SECTOR_W-120.0f),oy+rng_range(&r,100.0f,SECTOR_H-100.0f));
        }
        size_dims(&r,c->tier,&c->width,&c->height);
        c->angle=rng_range(&r,-PI,PI);
        c->accent=(u8)(rng_chance(&r,22)?1:0);
        c->kind=(u8)(rng_chance(&r,c->tier>=SIZE_LARGE?14:7)?1:0);
        c->geometryId=geometry_id(w,sx,sy,0x51A7D201u+(u32)i*0x9E3779B9u);
        c->priority=hash_u32(c->geometryId^0xA3C59AC3u);
        c->shape=(int)(rng_next(&r)%(c->tier==SIZE_BEHEMOTH?6u:9u));
        candidate_bounds(c);
    }
    return count;
}

static CandidateCacheEntry *cached_candidates(const World *w,int sx,int sy) {
    u32 h=hash_u32((u32)sx*0x9E3779B9u^(u32)sy*0x85EBCA6Bu^w->seed);
    CandidateCacheEntry *e=&candidateCache[h&(CANDIDATE_CACHE_SLOTS-1)];
    if (!e->valid||e->seed!=w->seed||e->sx!=sx||e->sy!=sy) {
        e->valid=true;
        e->seed=w->seed;
        e->sx=sx;
        e->sy=sy;
        e->count=build_candidates(w,sx,sy,e->candidate);
    }
    return e;
}

static bool overlaps(const ShapeCandidate *a,const ShapeCandidate *b) {
    return a->minx-SHAPE_GAP<b->maxx&&a->maxx+SHAPE_GAP>b->minx&&
           a->miny-SHAPE_GAP<b->maxy&&a->maxy+SHAPE_GAP>b->miny;
}

static bool candidate_beats(const ShapeCandidate *a,const ShapeCandidate *b) {
    if (a->starter!=b->starter) return a->starter;
    if (a->tier!=b->tier) return a->tier>b->tier;
    if (a->priority!=b->priority) return a->priority<b->priority;
    if (a->sx!=b->sx) return a->sx<b->sx;
    if (a->sy!=b->sy) return a->sy<b->sy;
    return a->index<b->index;
}

static bool candidate_allowed(const World *w,const ShapeCandidate *c) {
    for (int sy=c->sy-OVERLAP_SCAN_Y;sy<=c->sy+OVERLAP_SCAN_Y;++sy) {
        for (int sx=c->sx-OVERLAP_SCAN_X;sx<=c->sx+OVERLAP_SCAN_X;++sx) {
            CandidateCacheEntry *e=cached_candidates(w,sx,sy);
            for (int i=0;i<e->count;++i) {
                ShapeCandidate *o=&e->candidate[i];
                if (o->sx==c->sx&&o->sy==c->sy&&o->index==c->index) continue;
                if (overlaps(c,o)&&candidate_beats(o,c)) return false;
            }
        }
    }
    return true;
}

static void emit_shape(Sector *out,const ShapeCandidate *c) {
    float width=c->width;
    float height=c->height;
    float angle=c->angle;
    Vec2 center=c->center;
    int shape=c->shape;
    u8 kind=c->kind;
    u8 accent=c->accent;
    u32 gid=c->geometryId;

    if (shape==0) add_regular(out,center,width*0.50f,height*0.50f,3,angle,kind,accent,gid);
    else if (shape==1) add_rectangle(out,center,width,width,angle,kind,accent,gid);
    else if (shape==2) add_rectangle(out,center,width,height,angle,kind,accent,gid);
    else if (shape==3) add_regular(out,center,width*0.50f,height*0.50f,4,angle+PI*0.25f,kind,accent,gid);
    else if (shape==4) add_regular(out,center,width*0.50f,height*0.50f,5,angle,kind,accent,gid);
    else if (shape==5) add_regular(out,center,width*0.50f,height*0.50f,6,angle,kind,accent,gid);
    else if (shape==6) add_regular(out,center,width*0.50f,height*0.50f,8,angle,kind,accent,gid);
    else if (shape==7) add_trapezoid(out,center,width,height,angle,kind,accent,gid);
    else add_star(out,center,width*0.50f,height*0.50f,angle,kind,accent,gid);

    if (c->tier>=SIZE_GIGANTIC&&out->segCount<MAX_SEG_PER_SECTOR-12) {
        float inset=c->tier==SIZE_BEHEMOTH?0.72f:0.68f;
        if (shape==0) add_regular(out,center,width*0.50f*inset,height*0.50f*inset,3,angle,kind,accent,gid);
        else if (shape==1) add_rectangle(out,center,width*inset,width*inset,angle,kind,accent,gid);
        else if (shape==2) add_rectangle(out,center,width*inset,height*inset,angle,kind,accent,gid);
        else if (shape==3) add_regular(out,center,width*0.50f*inset,height*0.50f*inset,4,angle+PI*0.25f,kind,accent,gid);
        else if (shape==4) add_regular(out,center,width*0.50f*inset,height*0.50f*inset,5,angle,kind,accent,gid);
        else if (shape==5) add_regular(out,center,width*0.50f*inset,height*0.50f*inset,6,angle,kind,accent,gid);
        else if (shape==6) add_regular(out,center,width*0.50f*inset,height*0.50f*inset,8,angle,kind,accent,gid);
        else if (shape==7) add_trapezoid(out,center,width*inset,height*inset,angle,kind,accent,gid);
        else add_star(out,center,width*0.50f*inset,height*0.50f*inset,angle,kind,accent,gid);
    }
}

static void generate_sector(const World *w,int sx,int sy,Sector *out) {
    memset(out,0,sizeof(*out));
    out->sx=sx;
    out->sy=sy;
    ShapeCandidate candidates[MAX_SHAPES_PER_SECTOR];
    int count=build_candidates(w,sx,sy,candidates);
    for (int i=0;i<count;++i) {
        if (!candidate_allowed(w,&candidates[i])) continue;
        emit_shape(out,&candidates[i]);
        if (out->segCount>=MAX_SEG_PER_SECTOR-12) break;
    }
}

void world_cache_reset(void) {
    memset(cache,0,sizeof(cache));
    memset(candidateCache,0,sizeof(candidateCache));
    cacheStamp=1;
}

Sector *world_get_sector(const World *world,int sx,int sy) {
    SectorCacheEntry *slot=&cache[0];
    for (int i=0;i<SECTOR_CACHE_SLOTS;++i) {
        SectorCacheEntry *e=&cache[i];
        if (e->valid&&e->sx==sx&&e->sy==sy) {
            e->stamp=++cacheStamp;
            return &e->sector;
        }
        if (!e->valid) slot=e;
        else if (slot->valid&&e->stamp<slot->stamp) slot=e;
    }
    slot->valid=true;
    slot->sx=sx;
    slot->sy=sy;
    slot->stamp=++cacheStamp;
    generate_sector(world,sx,sy,&slot->sector);
    return &slot->sector;
}
