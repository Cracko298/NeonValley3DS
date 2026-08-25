#include "physics.h"
#include <math.h>
#include <string.h>
#include "config.h"
#include "math2d.h"
#include "rng.h"
#include "world.h"

static Sector *collisionCache[COLLISION_CACHE_MAX];

bool physics_action_down(u32 held) {
    return (held&(KEY_TOUCH|KEY_A|KEY_L|KEY_R))!=0;
}

static bool geometry_award(Game *game,u32 id) {
    if (!id) return false;
    u32 index=hash_u32(id)&(TOUCHED_GEOMETRY_SLOTS-1u);
    for (u32 i=0;i<TOUCHED_GEOMETRY_SLOTS;++i) {
        u32 *slot=&game->touchedGeometry[(index+i)&(TOUCHED_GEOMETRY_SLOTS-1u)];
        if (*slot==id) return false;
        if (*slot==0) {
            *slot=id;
            game->shots++;
            game->launchFlash=fmaxf(game->launchFlash,0.10f);
            return true;
        }
    }
    return false;
}

void physics_reset(Game *game) {
    game->shots=1;
    memset(game->touchedGeometry,0,sizeof(game->touchedGeometry));
    game->aimAngle=-0.62f;
    game->aiming=false;
    game->prevAction=false;
    game->launchFlash=0.0f;
    game->impactFlash=0.0f;
    game->cameraKick=0.0f;
    game->launchSlowTimer=0.0f;
    game->launches=0;
}

static void launch(Game *game) {
    if (game->shots<=0) return;
    Vec2 dir=v2(cosf(game->aimAngle),sinf(game->aimAngle));
    float before=len2(game->vel);
    float bonus=clampf(before*LAUNCH_SPEED_BONUS,0.0f,MAX_LAUNCH_BONUS);
    game->vel=add2(game->vel,mul2(dir,BASE_LAUNCH_FORCE+bonus));
    float speed=len2(game->vel);
    if (speed>MAX_SPEED&&speed>0.001f) game->vel=mul2(game->vel,MAX_SPEED/speed);
    game->shots--;
    game->launches++;
    game->launchFlash=0.24f;
    game->cameraKick=0.18f;
    game->launchSlowTimer=LAUNCH_SLOW_TIME;
}

static void collide(Game *game,const Segment *s) {
    Vec2 q=closest_on_segment(game->pos,s->a,s->b);
    Vec2 d=sub2(game->pos,q);
    float ds=len2sq(d);
    float contactRadius=PLAYER_RADIUS+GEOMETRY_HALF_THICKNESS;
    float rr=contactRadius*contactRadius;
    if (ds>=rr) return;
    geometry_award(game,s->geometryId);
    float dist=sqrtf(ds);
    Vec2 n;
    if (dist>0.0001f) n=mul2(d,1.0f/dist);
    else {
        Vec2 ab=sub2(s->b,s->a);
        float l=len2(ab);
        n=l<0.001f?v2(0,-1):v2(-ab.y/l,ab.x/l);
        if (dot2(game->vel,n)>0) n=mul2(n,-1.0f);
    }
    game->pos=add2(game->pos,mul2(n,contactRadius-dist+0.12f));
    float vn=dot2(game->vel,n);
    if (vn>=0.0f) return;
    float impact=-vn;
    float restitution=s->kind==1?BUMPER_RESTITUTION:NORMAL_RESTITUTION;
    game->vel=sub2(game->vel,mul2(n,(1.0f+restitution)*vn));
    Vec2 tangent=v2(-n.y,n.x);
    float vt=dot2(game->vel,tangent);
    float outNormal=dot2(game->vel,n);
    if (s->kind!=1&&outNormal>0.0f&&outNormal<SETTLE_NORMAL_SPEED) outNormal=0.0f;
    float keep=1.0f;
    if (impact>IMPACT_FRICTION_START) {
        float t=clampf((impact-IMPACT_FRICTION_START)/(IMPACT_FRICTION_FULL-IMPACT_FRICTION_START),0.0f,1.0f);
        float minKeep=s->kind==1?BUMPER_TANGENT_MIN:IMPACT_TANGENT_MIN;
        keep=1.0f-(1.0f-minKeep)*t;
    }
    game->vel=add2(mul2(n,outNormal),mul2(tangent,vt*keep));
    if (impact>180.0f) {
        game->impactFlash=clampf(impact/1000.0f,0.08f,0.22f);
        game->cameraKick=clampf(impact/1800.0f,0.05f,0.16f);
    }
}

static int build_collision_cache(Game *game) {
    int csx=(int)floorf(game->pos.x/SECTOR_W);
    int csy=(int)floorf(game->pos.y/SECTOR_H);
    int n=0;
    for (int sy=csy-COLLISION_RADIUS_Y;sy<=csy+COLLISION_RADIUS_Y;++sy)
        for (int sx=csx-COLLISION_RADIUS_X;sx<=csx+COLLISION_RADIUS_X;++sx)
            collisionCache[n++]=world_get_sector(&game->world,sx,sy);
    return n;
}

static void collide_world(Game *game,int count) {
    for (int i=0;i<count;++i) {
        Sector *sec=collisionCache[i];
        for (int j=0;j<sec->segCount;++j) {
            Segment *s=&sec->seg[j];
            if (game->pos.x<s->minx||game->pos.x>s->maxx||game->pos.y<s->miny||game->pos.y>s->maxy) continue;
            collide(game,s);
        }
    }
}

float physics_update(Game *game,float dt,u32 held) {
    bool action=physics_action_down(held);
    if (action&&!game->prevAction&&game->shots>0) game->aiming=true;
    if (game->aiming&&action) game->aimAngle=wrap_angle(game->aimAngle+AIM_SPEED*dt);
    if (!action&&game->prevAction&&game->aiming) {
        launch(game);
        game->aiming=false;
    }
    game->prevAction=action;

    float timeScale=1.0f;
    if (game->aiming&&action) timeScale=AIM_TIME_SCALE;
    else if (game->launchSlowTimer>0.0f) timeScale=LAUNCH_TIME_SCALE;
    float simDt=dt*timeScale;
    if (game->launchSlowTimer>0.0f) {
        game->launchSlowTimer-=dt;
        if (game->launchSlowTimer<0.0f) game->launchSlowTimer=0.0f;
    }

    float speed=len2(game->vel);
    int steps=2+(int)(speed/420.0f);
    if (steps<2) steps=2;
    if (steps>7) steps=7;
    float h=simDt/(float)steps;
    Vec2 oldPos=game->pos;
    int cacheCount=build_collision_cache(game);
    for (int i=0;i<steps;++i) {
        game->vel.y+=GRAVITY*h;
        game->pos=add2(game->pos,mul2(game->vel,h));
        collide_world(game,cacheCount);
    }
    float capped=len2(game->vel);
    if (capped>MAX_SPEED) game->vel=mul2(game->vel,MAX_SPEED/capped);
    game->distance+=len2(sub2(game->pos,oldPos));
    speed=len2(game->vel);
    if (speed>game->maxSpeedSeen) game->maxSpeedSeen=speed;
    game->score=(int)(game->distance*0.11f)+(int)(speed*0.025f);
    if (game->score>game->bestScore) game->bestScore=game->score;
    return simDt;
}
