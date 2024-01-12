#pragma once

// level 0 is the main menu
struct scene_manager_t {
	bool queue_level_load = false;
	int cur_level = 0;
	int level_to_load = -1;
};

void scene_manager_load_level(scene_manager_t& sm, int level_num);

