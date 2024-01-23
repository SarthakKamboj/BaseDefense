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
#include "gameplay/preview_manager.h"

extern globals_t globals;

void update() {

    if (globals.scene_manager.cur_level < 0) {

    } else if (globals.scene_manager.cur_level == MAIN_MENU_LEVEL) {
        if (get_if_key_clicked_on("play_game_btn")) {
            globals.scene_manager.queue_level_load = true;
            globals.scene_manager.level_to_load = LEVELS_DISPLAY;
        } else if (get_if_key_clicked_on("quit_btn")) {
            globals.running = false;
        }
    } else if (globals.scene_manager.cur_level == GAME_OVER_SCREEN_LEVEL) {
        if (get_if_key_clicked_on("Continue")) {
            globals.scene_manager.queue_level_load = true;
            globals.scene_manager.level_to_load = MAIN_MENU_LEVEL;
        }
    } else if (globals.scene_manager.cur_level == LEVELS_DISPLAY) {
        for (int i = 1; i <= 5; i++) {
            char container_name[16]{};
            sprintf(container_name, "%i_container", i);
            if (get_if_key_clicked_on(container_name)) {
                globals.scene_manager.queue_level_load = true;
                globals.scene_manager.level_to_load = LEVEL_1 + (i-1);
            }

            char bottom_border_name[32]{};
            sprintf(bottom_border_name, "%i_bottom_border", i);
            if (get_if_key_hovered_over(container_name)) {
                play_ui_anim_player(bottom_border_name, "move_up");
            } else {
                stop_ui_anim_player(bottom_border_name, "move_up");
            }
        }
    } else {
        update_camera();
        update_rigidbodies();
        update_image_anim_players();
        gos_update();
        update_store();
        update_preview_mode();
    }

    scene_manager_update(globals.scene_manager);
}