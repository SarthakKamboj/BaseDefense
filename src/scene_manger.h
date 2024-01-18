#pragma once

#include "constants.h"

// level 0 is the test ui level
struct scene_manager_t {
	bool queue_level_load = false;
	int cur_level = TEST_UI_LEVEL + 1;
	int level_to_load = -1;
};

void scene_manager_load_level(scene_manager_t& sm, int level_num);
void scene_manager_update(scene_manager_t& sm);

