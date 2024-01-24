#pragma once

#include "utils/time.h"
#include "constants.h"
#include "preview_manager.h"

#include "glm/glm.hpp"

#include <vector>

#define NUM_BASE_ATTACH_PTS 3
#define NUM_BASE_EXT_ATTACH_PTS 2

void init_preview_items();

struct base_t {
	int handle = -1;

	bool previewing = true;

	int transform_handle = -1;
	int quad_render_handle = -1;
	int rb_handle = -1;

	// int attach_pt_transform_handles[NUM_BASE_ATTACH_PTS] = {-1, -1, -1};
	// bool attach_pts_attached[NUM_BASE_ATTACH_PTS] = {false, false, false};
	int attachment_handles[NUM_BASE_ATTACH_PTS] = {-1, -1, -1};

	static const int WIDTH;
	static const int HEIGHT;
};
void init_preview_base();
void update_preview_base();
void create_base(glm::vec3 pos);
void update_base(base_t& base);

struct base_extension_t {
	int handle = -1;

	bool previewing = true;
	bool free = true;

	int transform_handle = -1;
	int quad_render_handle = -1;
	int rb_handle = -1;

	int attachment_handle = -1;

	int attachment_handles[NUM_BASE_EXT_ATTACH_PTS] = {-1, -1};

	static const int WIDTH;
	static const int HEIGHT;
};
void init_base_ext_preview();
void update_preview_base_ext();
int create_base_ext(glm::vec3 pos);
void update_base_ext(base_extension_t& attachment);
base_extension_t* get_base_ext();

enum ATTACHMENT_TYPE : int {
	ATTMNT_NONE = 0,
	ATTMNT_GUN = 1 << 0,
	ATTMNT_BASE_EXT = 1 << 2,
};
OR_ENUM_DECLARATION(ATTACHMENT_TYPE)

struct attachment_t {
	int handle = -1;
	int transform_handle = -1;
	int quad_render_handle = -1;
	bool attached = false;
	bool facing_left = false;

	ATTACHMENT_TYPE attachment_types = ATTMNT_NONE;

	static const int WIDTH;
	static const int HEIGHT;
};
int create_attachment(glm::vec3 pos, bool facing_left, ATTACHMENT_TYPE attmt_types, base_t* base, base_extension_t* base_ext);
void update_attachment(attachment_t& attachment);
attachment_t* get_attachment(int handle);

struct closest_enemy_t {
	int handle = -1;	
	int transform_handle = -1;
};

// TODO: review linear algebra transformations since gun will need to rotate around attachment point
// TODO: may need some sort of gameobject hierarchy
struct gun_t {
	int handle = -1;

	bool previewing = true;
	bool free = true;

	int transform_handle = -1;
	int quad_render_handle = -1;
	int rb_handle = -1;

	int attachment_handle = -1;
	bool facing_left = false;

	float fire_rate = 0;
	time_count_t time_since_last_fire = 0;

	closest_enemy_t closest_enemy;
	time_count_t last_retarget_time = 0;
	glm::vec2 last_target_dir = glm::vec2(0);
	glm::vec2 prev_enemy_last_target_dir = glm::vec2(0);

	static const int WIDTH;
	static const int HEIGHT;

	static const float RETARGET_ANIM_TIME;

	static std::vector<int> enemy_died_handles;
};
void init_preview_gun();
void create_attached_gun(int attachment_handle, bool facing_left, float fire_rate);
void update_attached_gun(gun_t& gun);

struct bullet_t {
	int handle = -1;

	glm::vec3 dir = glm::vec3(0);
	float speed = 0;
	time_count_t creation_time = 0;
	static const float ALIVE_TIME;

	int transform_handle = -1;
	int quad_render_handle = -1;
	int rb_handle = -1;

	static const int WIDTH;
	static const int HEIGHT;
};
void create_bullet(glm::vec3& start_pos, glm::vec3& move_dir, float speed);
void update_bullet(bullet_t& bullet);
void delete_bullet(bullet_t& bullet);

struct enemy_t {
	int handle = -1;

	int transform_handle = -1;
	int quad_render_handle = -1;
	int rb_handle = -1;

	static const int WIDTH;
	static const int HEIGHT;

	int dir = 1;
	int health = 100;

	float speed = 0;
};

void create_enemy(glm::vec3 pos, int dir, float speed);
void update_enemy(enemy_t& enemy);
void delete_enemy(int enemy_handle);

struct enemy_spawner_t {
	int handle = -1;

	int transform_handle = -1;
	int quad_render_handle = -1;

	static const int WIDTH;
	static const int HEIGHT;

	static const float TIME_BETWEEN_SPAWNS;
	time_count_t last_spawn_time = -TIME_BETWEEN_SPAWNS;
	time_count_t enemy_relative_time = 0;
};

void create_enemy_spawner(glm::vec3 pos);
void update_enemy_spawner(enemy_spawner_t& spawner);

struct score_t {
	int enemies_left_to_kill = 10;
};
void update_score();

/**
 * @brief Update all gameobjects
*/
void gos_update();
void delete_gos();