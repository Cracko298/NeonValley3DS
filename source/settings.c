#include "settings.h"

void settings_defaults(Settings *s) {
    s->bloomMode=1;
    s->zoomMode=1;
    s->particleMode=2;
    s->trailMode=2;
    s->screenShake=true;
    s->speedFx=true;
    s->minimalHud=false;
    s->musicVolume=4;
}

void settings_adjust(Game *game,int dir) {
    switch (game->settingsIndex) {
        case 0: game->settings.bloomMode=(game->settings.bloomMode+dir+3)%3; break;
        case 1: game->settings.zoomMode=(game->settings.zoomMode+dir+3)%3; break;
        case 2: game->settings.particleMode=(game->settings.particleMode+dir+3)%3; break;
        case 3: game->settings.trailMode=(game->settings.trailMode+dir+3)%3; break;
        case 4: game->settings.screenShake=!game->settings.screenShake; break;
        case 5: game->settings.speedFx=!game->settings.speedFx; break;
        case 6: game->settings.minimalHud=!game->settings.minimalHud; break;
        case 7: game->settings.musicVolume=(game->settings.musicVolume+dir+5)%5; break;
    }
}

const char *settings_bloom_name(int v) { return v==0?"OFF":(v==1?"LOW":"HIGH"); }
const char *settings_zoom_name(int v) { return v==0?"NORMAL":(v==1?"WIDE":"EXTREME"); }
const char *settings_particle_name(int v) { return v==0?"OFF":(v==1?"LOW":"HIGH"); }
const char *settings_trail_name(int v) { return v==0?"SHORT":(v==1?"LONG":"ULTRA"); }
const char *settings_music_name(int v) {
    static const char *names[]={"OFF","25%","50%","75%","100%"};
    if (v<0) v=0;
    if (v>4) v=4;
    return names[v];
}
