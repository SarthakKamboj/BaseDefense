#include "scene_manger.h"

#include "constants.h"
#include "ui/ui.h"
#include "transform/transform.h"
#include "gfx/quad.h"
#include "physics/physics.h"
#include "gameplay/gos.h"
#include "gameplay/store.h"
#include "utils/json.h"
#include "utils/io.h"
#include "globals.h"
#include "gameplay/preview_manager.h"
#include "gameplay/enemies.h"
#include "gameplay/gos_globals.h"

extern go_globals_t go_globals;

extern inventory_t inventory;
extern store_t store;
extern globals_t globals;

std::vector<levels_data_t> scene_manager_t::levels;
std::vector<bool> scene_manager_t::levels_unlocked;

void scene_manager_init() {
    char buffer[256]{};
    get_resources_folder_path(buffer);
    char levels_data_path[256]{};
    sprintf(levels_data_path, "%s\\%s\\levels.json", buffer, LEVELS_FOLDER);
	json_document_t* doc = json_read_document(levels_data_path);

    int num_levels = doc->root_node->children[0]->num_children;
    json_node_t** levels_nodes = doc->root_node->children[0]->children;
    for (int i = 0; i < num_levels; i++) {
        levels_data_t level;
        json_node_t* level_node = levels_nodes[i];
        for (int j = 0; j < level_node->num_children; j++) {
            json_node_t* level_node_parameter = level_node->children[j];
            json_string_t* key = level_node_parameter->key;
            json_string_t* val = level_node_parameter->value;
            if (strcmp(key->buffer, "enemies") == 0) {
                level.num_enemies_to_kill = atoi(val->buffer);
            } else if (strcmp(key->buffer, "level_name") == 0) {
                memcpy(level.level_name, val->buffer, val->length);
            } else if (strcmp(key->buffer, "time") == 0) {
                level.time = atof(val->buffer);
            } else if (strcmp(key->buffer, "air_spawners") == 0) {
                json_node_t* air_spawner_section = level_node_parameter;
                for (int k = 0; k < air_spawner_section->num_children; k++) {
                    json_node_t* spawn_info_node = air_spawner_section->children[k];
                    for (int m = 0; m < spawn_info_node->num_children; m++) {
                        json_node_t* air_spawn_param = spawn_info_node->children[m];
                        if (strcmp(air_spawn_param->key->buffer, "x") == 0) {
                            int x_pos = atoi(air_spawn_param->value->buffer);
                            level.air_spawner_xs.push_back(x_pos);
                        }
                    }
                }
            }  else if (strcmp(key->buffer, "ground_spawners") == 0) {
                json_node_t* ground_spawner_section = level_node_parameter;
                for (int k = 0; k < ground_spawner_section->num_children; k++) {
                    json_node_t* spawn_info_node = ground_spawner_section->children[k];
                    ground_spawn_info_t ground_spawners_info;
                    for (int m = 0; m < spawn_info_node->num_children; m++) {
                        json_node_t* ground_spawn_param = spawn_info_node->children[m];
                        if (strcmp(ground_spawn_param->key->buffer, "x") == 0) {
                            int x_pos = atoi(ground_spawn_param->value->buffer);
                            ground_spawners_info.x = x_pos;
                        } else if (strcmp(ground_spawn_param->key->buffer, "dir") == 0) {
                            if (strcmp(ground_spawn_param->value->buffer, "right") == 0) {
                                ground_spawners_info.move_dir = MOVE_DIR::RIGHT;
                            } else {
                                ground_spawners_info.move_dir = MOVE_DIR::LEFT;
                            }
                        }
                    }
                    level.ground_spawners_info.push_back(ground_spawners_info);
                }
            }
        }
        scene_manager_t::levels.push_back(level);
        scene_manager_t::levels_unlocked.push_back(false);
    }

    memset(levels_data_path, 0, strlen(levels_data_path));
    sprintf(levels_data_path, "%s\\%s\\unlocked_levels.txt", buffer, LEVELS_FOLDER);
    FILE* unlocked_levels_file = fopen(levels_data_path, "r");
    while (!feof(unlocked_levels_file)) {
        char level[10]{};
        fscanf(unlocked_levels_file, "%s\n", level);
        int level_int = atoi(level);
        scene_manager_t::levels_unlocked[level_int-1] = true;
    }
    fclose(unlocked_levels_file);

    // keep level 1 unlocked
    scene_manager_t::levels_unlocked[0] = true;
    json_free_document(doc);
}

void unload_level() {
    delete_transforms();
    delete_quad_renders();
    delete_rbs();
    delete_gos();
}

void unlock_level(int level) {
    game_assert_msg(level >= LEVEL_1 && level <= LEVEL_5, "level is not within the correct bounds");
    globals.scene_manager.levels_unlocked[level-1] = true;

    char buffer[256]{};
    get_resources_folder_path(buffer);
    char levels_data_path[256]{};
    memset(levels_data_path, 0, strlen(levels_data_path));
    sprintf(levels_data_path, "%s\\%s\\unlocked_levels.txt", buffer, LEVELS_FOLDER);
    FILE* unlocked_levels_file = fopen(levels_data_path, "w");
    for (int i = 0; i < level; i++) {
        fprintf(unlocked_levels_file, "%i\n", i+1);
    }
    fclose(unlocked_levels_file);
}

void scene_manager_update(scene_manager_t& sm) {
    if (sm.queue_level_load) {
        sm.queue_level_load = false;
        sm.cur_level = sm.level_to_load;
        sm.level_to_load = -1;
    
        unload_level();
        reset_ui_anim_player_cnt();
    
        clear_active_ui_files();
        clear_active_ui_anim_files();
        go_globals.paused = false;

        ui_enable_controller_support();

        if (sm.cur_level == TEST_UI_LEVEL) {
            add_active_ui_file("pause_menu.xml");
        } else if (sm.cur_level == MAIN_MENU_LEVEL) {
            add_active_ui_file("main_menu.xml");
            add_active_ui_anim_file("main_menu_anims.json");
        } else if (sm.cur_level == GAME_OVER_SCREEN_LEVEL) {
            add_active_ui_file("game_over.xml");
            add_active_ui_anim_file("game_over_anims.json");
        } else if (sm.cur_level == LOSE_SCREEN_LEVEL) {
            add_active_ui_file("lose.xml");
            add_active_ui_anim_file("lose_anims.json");
        } else if (sm.cur_level == LEVEL_COMPLETE_LEVEL) {
            add_active_ui_file("win_level.xml");
            add_active_ui_anim_file("win_level_anims.json");
        } else if (sm.cur_level == SETTINGS_MENU_LEVEL) {
            add_active_ui_file("settings.xml");
            add_active_ui_file("settings_anims.json");
        } else if (sm.cur_level == CREDITS_LEVEL) {
            add_active_ui_file("credits.xml");
            add_active_ui_file("credits_anims.json");
        } else if (sm.cur_level == LEVELS_DISPLAY) {
            add_active_ui_file("levels_display.xml");
            add_active_ui_anim_file("levels_display_anims.json");

            for (int i = 1; i <= 5; i++) {
                char bottom_border_name[32]{};
                sprintf(bottom_border_name, "%i_bottom_border", i);
                add_ui_anim_to_widget(bottom_border_name, "move_up");
            }
        } else {
            // scene_manager_t::levels_unlocked[sm.cur_level-1] = true;
            if (!scene_manager_t::levels_unlocked[sm.cur_level-1]) {
                unlock_level(sm.cur_level);
            }

            enemy_t::deleted_base_handles.clear();
            gun_t::enemy_died_handles.clear();

            init_preview_mode();
	        init_camera();	

            for (auto& info : scene_manager_t::levels[sm.cur_level-1].ground_spawners_info) {
                create_enemy_spawner(glm::vec2(info.x, 50), info.move_dir);
            }

            for (float x : scene_manager_t::levels[sm.cur_level-1].air_spawner_xs) {
                create_air_enemy_spawner(glm::vec2(x, 500));
            }

            // create_enemy_spawner(glm::vec2(50, 50), MOVE_DIR::RIGHT);
            // create_enemy_spawner(glm::vec2(1000, 50), MOVE_DIR::LEFT);
            // create_air_enemy_spawner(glm::vec2(1000, 500));

            add_active_ui_file("play.xml");
            add_active_ui_file("play_anims.json");

            add_active_ui_file("store.xml");
            add_active_ui_anim_file("store_anims.json");
            set_translate_in_ui_anim("store_close", glm::vec2(-globals.window.window_width * 0.829f, 0));
            add_ui_anim_to_widget("store_panel", "store_close", 1, true);

            add_active_ui_file("pause_menu.xml");
            add_active_ui_anim_file("pause_menu_anims.json");
            set_translate_in_ui_anim("pause_menu_close", glm::vec2(0, globals.window.window_height));
            add_ui_anim_to_widget("pause_panel", "pause_menu_close", 1, true);

            add_ui_anim_to_widget("base_container", "selected");
            add_ui_anim_to_widget("base_ext_container", "selected");
            add_ui_anim_to_widget("gun_container", "selected");

            go_globals.score.enemies_left_to_kill = scene_manager_t::levels[sm.cur_level-1].num_enemies_to_kill;
            inventory = inventory_t();
            store = store_t();
            go_globals.time_left = scene_manager_t::levels[sm.cur_level-1].time;

            ui_disable_controller_support();
        }
    } 
}