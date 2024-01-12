#include "scene_manger.h"

#include "constants.h"

void scene_manager_load_level(scene_manager_t& sm, int level_num) {
    char msg[32]{};
    sprintf(msg, "loading level %i", level_num);
    game_info_log(msg);

	sm.queue_level_load = true;
	sm.level_to_load = level_num;
}

