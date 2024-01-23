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

extern score_t score;
extern inventory_t inventory;
extern store_t store;


std::vector<levels_data_t> scene_manager_t::levels;

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
            }
        }
        scene_manager_t::levels.push_back(level);
    }

    json_free_document(doc);
    int a = 5;
}

void unload_level() {
    delete_transforms();
    delete_quad_renders();
    delete_rbs();
    delete_gos();
}

void scene_manager_update(scene_manager_t& sm) {
    if (sm.queue_level_load) {
        sm.queue_level_load = false;
        sm.cur_level = sm.level_to_load;
        sm.level_to_load = -1;
    
        unload_level();
    
        clear_active_ui_files();
        clear_active_ui_anim_files();

        if (sm.cur_level == MAIN_MENU_LEVEL) {
            add_active_ui_file("main_menu.xml");
            add_active_ui_anim_file("main_menu_anims.json");
        } else if (sm.cur_level == GAME_OVER_SCREEN_LEVEL) {
            add_active_ui_file("game_over.xml");
        } else if (sm.cur_level == LEVELS_DISPLAY) {
            add_active_ui_file("levels_display.xml");
            add_active_ui_anim_file("levels_display_anims.json");

            for (int i = 1; i <= 5; i++) {
                char bottom_border_name[32]{};
                sprintf(bottom_border_name, "%i_bottom_border", i);
                add_ui_anim_to_widget(bottom_border_name, "move_up");
            }
        } else {
            init_preview_items();
            create_enemy_spawner(glm::vec3(50, 50, 0));
            add_active_ui_file("play.xml");
            add_active_ui_file("store.xml");
            add_active_ui_anim_file("store_anims.json");
            score.enemies_left_to_kill = scene_manager_t::levels[sm.cur_level-1].num_enemies_to_kill;
            inventory = inventory_t();
            store = store_t();
        }
    } 
}