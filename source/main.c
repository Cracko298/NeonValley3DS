#include <3ds.h>
#include "audio.h"
#include "game.h"
#include "math2d.h"
#include "physics.h"
#include "renderer.h"
#include "settings.h"
#include "ui.h"

int main(int argc,char **argv) {
    (void)argc; (void)argv;
    gfxInitDefault();
    gfxSet3D(false);

    Game game;
    game_init(&game);
    renderer_boot();
    gfxFlushBuffers();
    gfxSwapBuffers();
    gspWaitForVBlank();
    for (int i=0;i<3;++i) gspWaitForVBlank();

    ui_init();
    audio_init();
    audio_set_volume(game.settings.musicVolume);

    u64 last=osGetTime();
    while (aptMainLoop()) {
        hidScanInput();
        u32 down=hidKeysDown(),held=hidKeysHeld();
        if (down&KEY_START) break;

        if (down&KEY_SELECT) {
            game.settingsOpen=!game.settingsOpen;
            game.aiming=false;
            game.prevAction=physics_action_down(held);
        }

        if (game.settingsOpen) {
            if (down&KEY_DUP) game.settingsIndex=(game.settingsIndex+7)%8;
            if (down&KEY_DDOWN) game.settingsIndex=(game.settingsIndex+1)%8;
            if (down&KEY_DLEFT) settings_adjust(&game,-1);
            if (down&(KEY_DRIGHT|KEY_A)) settings_adjust(&game,1);
            audio_set_volume(game.settings.musicVolume);
        } else if (game.state==STATE_TITLE) {
            if (down&(KEY_A|KEY_TOUCH|KEY_L|KEY_R|KEY_X)) {
                game_new_run(&game);
                game.prevAction=physics_action_down(held);
            }
        } else if (game.state==STATE_DEAD) {
            if (down&(KEY_A|KEY_TOUCH|KEY_L|KEY_R|KEY_X)) {
                game_new_run(&game);
                game.prevAction=physics_action_down(held);
            }
        }

        u64 now=osGetTime();
        float dt=clampf((now-last)/1000.0f,1.0f/120.0f,1.0f/30.0f);
        last=now;
        if (!game.settingsOpen) game_update(&game,dt,held);
        audio_update();
        renderer_draw(&game);
        ui_update(&game);
        gfxFlushBuffers();
        gfxSwapBuffers();
        gspWaitForVBlank();
    }

    audio_exit();
    gfxExit();
    return 0;
}
