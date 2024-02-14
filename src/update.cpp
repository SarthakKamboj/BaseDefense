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
#include "gameplay/gos_globals.h"

extern globals_t globals;
extern go_globals_t go_globals;
extern store_t store;

void update() {

    time_count_t delta_time = game::time_t::delta_time;
    if (globals.scene_manager.cur_level < 0) {

    } else if (globals.scene_manager.cur_level == MAIN_MENU_LEVEL) {
        if (get_if_key_clicked_on("play_game_btn")) {
            globals.scene_manager.queue_level_load = true;
            globals.scene_manager.level_to_load = LEVELS_DISPLAY;
        } else if (get_if_key_clicked_on("settings_btn")) {
            globals.scene_manager.queue_level_load = true;
            globals.scene_manager.level_to_load = SETTINGS_MENU_LEVEL;
        } else if (get_if_key_clicked_on("quit_btn")) {
            globals.running = false;
        }
    } else if (globals.scene_manager.cur_level == SETTINGS_MENU_LEVEL) {
        set_ui_value(std::string("mute_text"), std::string(globals.muted ? "Unmute" : "Mute"));
        set_ui_value(std::string("fs_text"), std::string(globals.fullscreen ? "No Full Screen" : "Full Screen"));

        if (get_if_key_clicked_on("back_btn")) {
            globals.scene_manager.queue_level_load = true;
            globals.scene_manager.level_to_load = MAIN_MENU_LEVEL;
        } else if (get_if_key_clicked_on("mute_btn")) {
            globals.muted = !globals.muted;
        } else if (get_if_key_clicked_on("fs_btn")) {
            globals.fullscreen = !globals.fullscreen;
        } else if (get_if_key_clicked_on("credits_btn")) {
            globals.scene_manager.queue_level_load = true;
            globals.scene_manager.level_to_load = CREDITS_LEVEL;
        }
    } else if (globals.scene_manager.cur_level == CREDITS_LEVEL) {
        if (get_if_key_clicked_on("back_btn")) {
            globals.scene_manager.queue_level_load = true;
            globals.scene_manager.level_to_load = SETTINGS_MENU_LEVEL;
        } 
    } else if (globals.scene_manager.cur_level == GAME_OVER_SCREEN_LEVEL) {
        if (get_if_key_clicked_on("continue_btn")) {
            globals.scene_manager.queue_level_load = true;
            globals.scene_manager.level_to_load = MAIN_MENU_LEVEL;
        }
    } else if (globals.scene_manager.cur_level == LOSE_SCREEN_LEVEL) {
        if (get_if_key_clicked_on("continue_btn")) {
            globals.scene_manager.queue_level_load = true;
            globals.scene_manager.level_to_load = MAIN_MENU_LEVEL;
        }
    } else if (globals.scene_manager.cur_level == LEVEL_COMPLETE_LEVEL) {
        if (get_if_key_clicked_on("continue_btn")) {
            globals.scene_manager.queue_level_load = true;
            globals.scene_manager.level_to_load = LEVELS_DISPLAY;
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

        if (get_if_key_clicked_on("home_btn")) {
            globals.scene_manager.queue_level_load = true;
            globals.scene_manager.level_to_load = MAIN_MENU_LEVEL;
        }
    } else {

        bool prev_paused = go_globals.paused;
        update_store();

        if (globals.window.resized) {
            set_translate_in_ui_anim("pause_menu_close", glm::vec2(0, globals.window.window_height));
        }

        set_ui_value(std::string("pause_text"), std::string("| |"));
        set_ui_value(std::string("space"), std::string(" "));
        if (get_if_key_clicked_on("pause_icon_btn")) {
            go_globals.paused = true;
        }

        if (get_released(KEY_P) || get_pressed(CONTROLLER_START)) {
            go_globals.paused = !go_globals.paused;     
        }

        if (get_if_key_clicked_on("resume_btn")) {
            go_globals.paused = false;
        } else if (get_if_key_clicked_on("back_to_main_menu_btn")) {
            globals.scene_manager.queue_level_load = true;
            globals.scene_manager.level_to_load = MAIN_MENU_LEVEL;
        }

        if (!go_globals.paused) {
            update_camera();
            update_rigidbodies();
            update_image_anim_players();
            gos_update();
            if (!go_globals.paused) {
                go_globals.time_left -= game::time_t::delta_time;
            }
            set_ui_value(std::string("time_left"), std::to_string((int)ceil(go_globals.time_left)));
            if (go_globals.time_left <= 0) {
                globals.scene_manager.queue_level_load = true;
                globals.scene_manager.level_to_load = LOSE_SCREEN_LEVEL;
            }
            game::time_t::game_cur_time += delta_time;
        }

        if (prev_paused != go_globals.paused) {
            if (go_globals.paused) {
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

    scene_manager_update(globals.scene_manager);
}