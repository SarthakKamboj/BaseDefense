#include "scene_manger.h"

#include "constants.h"
#include "ui/ui.h"

// void scene_manager_load_level(scene_manager_t& sm, int level_num) {
//     char msg[32]{};
//     sprintf(msg, "loading level %i", level_num);
//     game_info_log(msg);

// 	sm.queue_level_load = true;
// 	sm.level_to_load = level_num;
// }

void scene_manager_update(scene_manager_t& sm) {
    if (sm.queue_level_load) {
        sm.queue_level_load = false;
        sm.cur_level = sm.level_to_load;
        sm.level_to_load = -1;
    
        clear_active_ui_files();
        if (sm.cur_level == MAIN_MENU_LEVEL) {
            add_active_ui_file("main_menu.xml");
        } else if (sm.cur_level == GAME_OVER_SCREEN_LEVEL) {
            add_active_ui_file("game_over.xml");
        } else {
            add_active_ui_file("play.xml");
            add_active_ui_file("store.xml");
        }
    } 
}