#pragma once

#include "utils/time.h"
#include "go_helper.h"

#include "glm/glm.hpp"

#include <vector>

enum class ENEMY_STATE {
	ENEMY_MOVING = 0,
	ENEMY_SHOOTING
};

enum class MOVE_DIR {
	RIGHT = 1,
	LEFT = -1,

	NONE = 0
};

enum class ENEMY_TYPE {
	GROUND = 0,
	AIR
};

struct air_enemy_specific_info_t {
	glm::vec2 target_loc_at_top_of_screen = glm::vec2(-1, -1);
	bool at_top_of_screen = false;
	MOVE_DIR dir = MOVE_DIR::RIGHT;
	time_count_t last_time_changed_dir = 0;
};

struct enemy_t {
	int handle = -1;
	ENEMY_STATE enemy_state = ENEMY_STATE::ENEMY_MOVING;
	ENEMY_TYPE enemy_type = ENEMY_TYPE::GROUND;

	int transform_handle = -1;
	int quad_render_handle = -1;
	int rb_handle = -1;

	static const int GROUND_WIDTH;
	static const int GROUND_HEIGHT;
	static const int AIR_WIDTH;
	static const int AIR_HEIGHT;	

	air_enemy_specific_info_t air;

	int width = -1;
	int height = -1;

	MOVE_DIR dir = MOVE_DIR::RIGHT;
	int health = 100;

	float speed = 0;
	closest_entity_t closest_base;
	static const float DIST_TO_BASE;
	static const float TIME_BETWEEN_SHOTS;
	time_count_t last_shoot_time = -TIME_BETWEEN_SHOTS;

	static std::vector<int> deleted_base_handles;
};

void create_enemy(glm::vec2 pos, MOVE_DIR dir, float speed);
void create_air_enemy(glm::vec2 pos, float speed);
void update_enemy(enemy_t& enemy);
void delete_enemy_by_index(int idx, const int handle);
void delete_enemy(int enemy_handle);

struct enemy_spawner_t {
	int handle = -1;

	int transform_handle = -1;
	int quad_render_handle = -1;

	MOVE_DIR dir = MOVE_DIR::RIGHT;
	ENEMY_TYPE enemy_type_spawner = ENEMY_TYPE::GROUND;

	static const int WIDTH;
	static const int HEIGHT;

	static const float TIME_BETWEEN_SPAWNS;
	time_count_t last_spawn_time = -TIME_BETWEEN_SPAWNS;
	time_count_t enemy_relative_time = 0;
};
void create_enemy_spawner(glm::vec2 pos, MOVE_DIR dir);
void create_air_enemy_spawner(glm::vec2 pos);
void update_enemy_spawner(enemy_spawner_t& spawner);
void delete_enemy_spawner(enemy_spawner_t& spawner);