#include "game.h"
#include <string.h>
#include "config.h"
#include "darkness.h"
#include "math2d.h"
#include "physics.h"
#include "rng.h"
#include "settings.h"
#include "world.h"

static Color palette(int phase) {
    static const Color p[]={{190,18,255},{20,235,255},{50,255,80},{255,120,16},{255,28,116}};
    phase%=5; if (phase<0) phase+=5; return p[phase];
}

Color game_neon(const Game *game) {
    int phase=(int)(game->distance/7200.0f); if (phase<0) phase=0; return palette(phase%5);
}

static void reset_trail(Game *game) {
    game->trailHead=0; game->trailCount=0; game->lastTrailMs=osGetTime();
    for (int i=0;i<TRAIL_POINTS;++i) game->trail[i]=game->pos;
}

void game_init(Game *game) {
    memset(game,0,sizeof(*game));
    settings_defaults(&game->settings);
    game->settingsIndex=0;
    game->seed=rng_run_seed(0);
    game->world.seed=game->seed;
    game->state=STATE_TITLE;
    game->pos=v2(SECTOR_W*0.5f,SECTOR_H*0.5f);
    game->vel=v2(START_SPEED,-42.0f);
    game->cameraX=game->pos.x+70.0f;
    game->cameraY=game->pos.y+4.0f;
    game->cameraScale=CAMERA_SCALE_NEAR;
    game->shield=DARKNESS_SHIELD_MAX;
    game->shots=1;
    world_cache_reset();
    reset_trail(game);
}

void game_new_run(Game *game) {
    game->seed=rng_run_seed(game->seed);
    game->world.seed=game->seed;
    game->state=STATE_PLAYING;
    game->settingsOpen=false;
    game->pos=v2(SECTOR_W*0.5f,SECTOR_H*0.5f);
    game->vel=v2(START_SPEED,-42.0f);
    game->maxSpeedSeen=len2(game->vel);
    game->distance=0.0f;
    game->score=0;
    physics_reset(game);
    darkness_reset(game);
    game->cameraX=game->pos.x+70.0f;
    game->cameraY=game->pos.y+4.0f;
    game->cameraScale=CAMERA_SCALE_NEAR;
    world_cache_reset();
    reset_trail(game);
}

static void update_camera(Game *game,float dt) {
    float speed=len2(game->vel);
    float ratio=clampf(speed/MAX_SPEED,0.0f,1.0f);
    float z=clampf((ratio-0.06f)/0.94f,0.0f,1.0f);
    z=powf(z,0.70f);
    float farScale=game->settings.zoomMode==0?0.20f:(game->settings.zoomMode==2?0.10f:CAMERA_SCALE_FAR);
    float nearScale=game->settings.zoomMode==2?0.72f:CAMERA_SCALE_NEAR;
    float targetScale=nearScale+(farScale-nearScale)*z;
    game->cameraScale+=(targetScale-game->cameraScale)*clampf(dt*5.4f,0.0f,1.0f);
    float look=0.08f+0.31f*z;
    Vec2 target=add2(game->pos,mul2(game->vel,look));
    if (game->settings.screenShake&&game->cameraKick>0.0f) {
        u32 k=hash_u32((u32)osGetTime());
        target.x+=((int)(k&255)-127)/127.0f*10.0f*game->cameraKick;
        target.y+=((int)((k>>8)&255)-127)/127.0f*10.0f*game->cameraKick;
    }
    if (game->cameraKick>0.0f) { game->cameraKick-=dt; if (game->cameraKick<0.0f) game->cameraKick=0.0f; }
    game->cameraX+=(target.x-game->cameraX)*clampf(dt*5.0f,0.0f,1.0f);
    game->cameraY+=(target.y-game->cameraY)*clampf(dt*5.0f,0.0f,1.0f);
}

static void update_trail(Game *game) {
    u64 now=osGetTime();
    if (now-game->lastTrailMs<TRAIL_INTERVAL_MS) return;
    game->trail[game->trailHead]=game->pos;
    game->trailHead=(game->trailHead+1)%TRAIL_POINTS;
    if (game->trailCount<TRAIL_POINTS) game->trailCount++;
    game->lastTrailMs=now;
}

void game_update(Game *game,float dt,u32 held) {
    if (game->state!=STATE_PLAYING) return;
    float simDt=physics_update(game,dt,held);
    darkness_update(game,simDt);
    update_camera(game,dt);
    if (game->launchFlash>0.0f) game->launchFlash-=dt;
    if (game->impactFlash>0.0f) game->impactFlash-=dt;
    update_trail(game);
}
