#include "scene_manger.h"

#include "constants.h"

void scene_manager_load_level(scene_manager_t& sm, int level_num) {
    char msg[32]{};
    sprintf(msg, "loading level %i", level_num);
    game_info_log(msg);

	sm.queue_level_load = true;
	sm.level_to_load = level_num;
}

void scene_manager_update(scene_manager_t& sm) {
    if (sm.queue_level_load) {
        sm.queue_level_load = false;
        sm.cur_level = sm.level_to_load;
        sm.level_to_load = -1;
    }
}