#include "darkness.h"
#include "config.h"
#include "math2d.h"

void darkness_reset(Game *game) {
    game->darknessActive=false;
    game->darknessPos=v2(-DARKNESS_START_GAP,0.0f);
    game->darknessSpeed=DARKNESS_BASE_SPEED;
    game->darknessDanger=0.0f;
    game->shield=DARKNESS_SHIELD_MAX;
}

void darkness_update(Game *game,float dt) {
    float speed=len2(game->vel);
    if (speed>game->maxSpeedSeen) game->maxSpeedSeen=speed;
    if (!game->darknessActive&&game->distance>=DARKNESS_START_DISTANCE) {
        Vec2 dir=speed>0.001f?mul2(game->vel,1.0f/speed):v2(1.0f,0.0f);
        game->darknessPos=sub2(game->pos,mul2(dir,DARKNESS_START_GAP));
        game->darknessSpeed=DARKNESS_BASE_SPEED;
        game->darknessActive=true;
    }
    if (!game->darknessActive) {
        game->darknessDanger=0.0f;
        return;
    }
    Vec2 toPlayer=sub2(game->pos,game->darknessPos);
    float dist=len2(toPlayer);
    float difficulty=clampf(game->distance/30000.0f,0.0f,1.0f);
    float speedTerm=clampf(game->maxSpeedSeen/MAX_SPEED,0.0f,1.0f);
    float target=DARKNESS_BASE_SPEED+difficulty*165.0f+speedTerm*105.0f;
    game->darknessSpeed+=(target-game->darknessSpeed)*clampf(dt*0.48f,0.0f,1.0f);
    if (dist>0.001f) game->darknessPos=add2(game->darknessPos,mul2(toPlayer,game->darknessSpeed*dt/dist));
    dist=len2(sub2(game->pos,game->darknessPos));
    game->darknessDanger=1.0f-clampf((dist-DARKNESS_RADIUS)/(DARKNESS_WARNING_RADIUS-DARKNESS_RADIUS),0.0f,1.0f);
    if (dist<DARKNESS_RADIUS) {
        float depth=1.0f-clampf(dist/DARKNESS_RADIUS,0.0f,1.0f);
        game->shield-=DARKNESS_DAMAGE_RATE*(0.75f+depth*0.75f)*dt;
        game->impactFlash=fmaxf(game->impactFlash,0.06f+depth*0.10f);
    } else if (dist>DARKNESS_WARNING_RADIUS) {
        game->shield+=DARKNESS_SHIELD_REGEN*dt;
    }
    game->shield=clampf(game->shield,0.0f,DARKNESS_SHIELD_MAX);
    if (game->shield<=0.0f) {
        game->state=STATE_DEAD;
        game->aiming=false;
    }
}
