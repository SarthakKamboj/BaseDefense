#include "scene_manger.h"

#include "constants.h"
#include "ui/ui.h"
#include "transform/transform.h"
#include "gfx/quad.h"
#include "physics/physics.h"
#include "gameplay/gos.h"
#include "gameplay/store.h"


extern score_t score;
extern inventory_t inventory;
extern store_t store;

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
        if (sm.cur_level == MAIN_MENU_LEVEL) {
            add_active_ui_file("main_menu.xml");
        } else if (sm.cur_level == GAME_OVER_SCREEN_LEVEL) {
            add_active_ui_file("game_over.xml");
        } else if (sm.cur_level == LEVELS_DISPLAY) {
            add_active_ui_file("levels_display.xml");
        } else {
            init_preview_items();
            create_enemy_spawner(glm::vec3(50, 50, 0));
            add_active_ui_file("play.xml");
            add_active_ui_file("store.xml");
            score.enemies_left_to_kill = 10 * sm.cur_level;
            inventory = inventory_t();
            store = store_t();
        }
    } 
}