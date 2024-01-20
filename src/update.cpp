#include "update.h"

#include <cassert>

#include "utils/time.h"
#include "animation/animation.h"
#include "physics/physics.h"
#include "gameplay/gos.h"
#include "camera.h"
#include "globals.h"
#include "gameplay/store.h"
#include "ui/ui.h"

extern globals_t globals;

void update() {

    if (globals.scene_manager.cur_level < 0) {

    } else if (globals.scene_manager.cur_level == MAIN_MENU_LEVEL) {
        if (get_if_key_clicked_on("Play Game")) {
            globals.scene_manager.queue_level_load = true;
            globals.scene_manager.level_to_load = LEVEL_1;
        } else if (get_if_key_clicked_on("Quit")) {
            globals.running = false;
        }
    } else if (globals.scene_manager.cur_level == GAME_OVER_SCREEN_LEVEL) {
        if (get_if_key_clicked_on("Continue")) {
            globals.scene_manager.queue_level_load = true;
            globals.scene_manager.level_to_load = MAIN_MENU_LEVEL;
        }
    } else if (globals.scene_manager.cur_level == LEVELS_DISPLAY) {
        
    } else {
        update_camera();
        update_rigidbodies();
        update_image_anim_players();
        gos_update();
        update_store();
    }

    scene_manager_update(globals.scene_manager);
}