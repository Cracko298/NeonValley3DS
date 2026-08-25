#include "ui.h"
#include <3ds.h>
#include <stdio.h>
#include "settings.h"

static PrintConsole console;
static int view=-1;

static void center_line(int row,const char *text) {
    int len=0;
    while (text[len]) ++len;
    int col=(40-len)/2;
    if (col<1) col=1;
    printf("\x1b[%d;%dH%s",row,col,text);
}

static void gameplay_help(void) {
    printf("\x1b[35m");
    center_line(4,"PRESS SELECT FOR SETTINGS");
    printf("\x1b[0m");
    center_line(23,"HOLD A / L / R / TOUCH TO AIM");
    center_line(25,"RELEASE TO LAUNCH");
    center_line(27,"START TO EXIT");
}

static void settings_view(Game *g) {
    printf("\x1b[2;6H\x1b[35mNEON VALLEY SETTINGS\x1b[0m");
    printf("\x1b[5;3H%c Bloom       %-8s",g->settingsIndex==0?'>':' ',settings_bloom_name(g->settings.bloomMode));
    printf("\x1b[7;3H%c Speed zoom  %-8s",g->settingsIndex==1?'>':' ',settings_zoom_name(g->settings.zoomMode));
    printf("\x1b[9;3H%c Particles   %-8s",g->settingsIndex==2?'>':' ',settings_particle_name(g->settings.particleMode));
    printf("\x1b[11;3H%c Trail       %-8s",g->settingsIndex==3?'>':' ',settings_trail_name(g->settings.trailMode));
    printf("\x1b[13;3H%c Shake       %-8s",g->settingsIndex==4?'>':' ',g->settings.screenShake?"ON":"OFF");
    printf("\x1b[15;3H%c Speed FX    %-8s",g->settingsIndex==5?'>':' ',g->settings.speedFx?"ON":"OFF");
    printf("\x1b[17;3H%c Minimal HUD %-8s",g->settingsIndex==6?'>':' ',g->settings.minimalHud?"ON":"OFF");
    printf("\x1b[19;3H%c Music       %-8s",g->settingsIndex==7?'>':' ',settings_music_name(g->settings.musicVolume));
    printf("\x1b[22;3HD-Pad  select / change");
    printf("\x1b[24;3HSELECT  close");
}

void ui_init(void) {
    consoleInit(GFX_BOTTOM,&console);
    consoleSelect(&console);
    consoleClear();
    view=-1;
}

void ui_update(Game *g) {
    consoleSelect(&console);
    int next=g->settingsOpen?1:0;
    bool changed=next!=view;
    if (changed) {
        consoleClear();
        view=next;
    }
    if (g->settingsOpen) settings_view(g);
    else gameplay_help();
}
