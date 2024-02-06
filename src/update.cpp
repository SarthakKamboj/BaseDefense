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
extern store_t store;

bool paused = false;
void update() {

    time_count_t delta_time = game::time_t::delta_time;
    if (globals.scene_manager.cur_level < 0) {

    } else if (globals.scene_manager.cur_level == MAIN_MENU_LEVEL) {
        if (get_if_key_clicked_on("play_game_btn")) {
            globals.scene_manager.queue_level_load = true;
            globals.scene_manager.level_to_load = LEVELS_DISPLAY;
        } else if (get_if_key_clicked_on("quit_btn")) {
            globals.running = false;
        }
    } else if (globals.scene_manager.cur_level == GAME_OVER_SCREEN_LEVEL) {
        if (get_if_key_clicked_on("continue_btn")) {
            globals.scene_manager.queue_level_load = true;
            globals.scene_manager.level_to_load = MAIN_MENU_LEVEL;
        }
    } else if (globals.scene_manager.cur_level == LEVELS_DISPLAY) {
        for (int i = 1; i <= 5; i++) {
            char text_buffer[32]{};
            sprintf(text_buffer, "%i_text", i);
            if (globals.scene_manager.levels_unlocked[i-1]) {
                set_ui_value(std::string(text_buffer), std::to_string(i));
            } else {
                set_ui_value(std::string(text_buffer), std::string("x"));
            }

            if (globals.scene_manager.levels_unlocked[i-1]) {
                char container_name[16]{};
                sprintf(container_name, "%i_container", i);
                if (get_if_key_clicked_on(container_name)) {
                    globals.scene_manager.queue_level_load = true;
                    globals.scene_manager.level_to_load = LEVEL_1 + (i-1);
                }

                char bottom_border_name[32]{};
                sprintf(bottom_border_name, "%i_bottom_border", i);
                if (get_if_key_mouse_enter(container_name)) {
                    play_ui_anim_player(bottom_border_name, "move_up");
                } else if (get_if_key_mouse_left(container_name)) {
                    stop_ui_anim_player(bottom_border_name, "move_up");
                }
            }
        }
    } else {

        bool prev_paused = paused;
        update_store();

        if (globals.window.resized) {
            set_translate_in_ui_anim("pause_menu_close", glm::vec2(0, globals.window.window_height));
        }

        set_ui_value(std::string("pause_text"), std::string("| |"));
        set_ui_value(std::string("space"), std::string(" "));
        if (get_if_key_clicked_on("pause_icon_btn")) {
            paused = true;
        }

        if (get_released(KEY_P) || get_pressed(CONTROLLER_START)) {
            paused = !paused;     
        }

        if (get_if_key_clicked_on("resume_btn")) {
            paused = false;
        } else if (get_if_key_clicked_on("back_to_main_menu_btn")) {
            globals.scene_manager.queue_level_load = true;
            globals.scene_manager.level_to_load = MAIN_MENU_LEVEL;
        }

        if (!paused) {
            update_camera();
            update_rigidbodies();
            update_image_anim_players();
            gos_update();
        } else {
            delta_time = 0;
        }

        if (prev_paused != paused) {
            if (paused) {
                stop_ui_anim_player("pause_panel", "pause_menu_close");
                ui_enable_controller_support();
            } else {
                play_ui_anim_player("pause_panel", "pause_menu_close");
                if (!store.open) {
                    ui_disable_controller_support();
                }
            }
        }
    }

    game::time_t::game_cur_time += delta_time;
    scene_manager_update(globals.scene_manager);
}