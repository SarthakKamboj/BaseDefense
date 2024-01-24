#pragma once

#include "constants.h"

#include <vector>

struct levels_data_t {
	int num_enemies_to_kill = 0;
	char level_name[64]{};
};

// level 0 is the test ui level
struct scene_manager_t {
	bool queue_level_load = true;
	int cur_level = -1;
	int level_to_load = MAIN_MENU_LEVEL;
	static std::vector<levels_data_t> levels;
	static std::vector<bool> levels_unlocked;
};

void scene_manager_init();
void unload_level();
// void scene_manager_load_level(scene_manager_t& sm, int level_num);
void scene_manager_update(scene_manager_t& sm);

