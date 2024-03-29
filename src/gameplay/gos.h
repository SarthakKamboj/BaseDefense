#pragma once

#include "utils/time.h"
#include "constants.h"
#include "go_helper.h"

#include "glm/glm.hpp"

#include <vector>
#include <unordered_map>

#define NUM_BASE_ATTACH_PTS 3
#define MAX_NUM_BASE_EXT_ATTACH_PTS 3

struct base_t {
	int handle = -1;

	int base_health = 100;

	int transform_handle = -1;
	int quad_render_handle = -1;
	int rb_handle = -1;

	int attachment_handles[NUM_BASE_ATTACH_PTS] = {-1, -1, -1};

	static const int WIDTH;
	static const int HEIGHT;
};
void create_base(glm::vec2 pos);
void update_base(base_t& base);
void delete_base(base_t& base);

enum BASE_EXT_TYPE {
	THREE_ATT_BOTTOM_GONE = 0,
	THREE_ATT_RIGHT_GONE,
	THREE_ATT_LEFT_GONE,
	TWO_ATT_HORIZONTAL,
	ONE_LEFT,
	ONE_RIGHT,
	
	NUM_BASE_EXT_TYPES,
};

struct base_extension_t {
	int handle = -1;

	int transform_handle = -1;
	int quad_render_handle = -1;
	int rb_handle = -1;

	int attachment_handle = -1;

	int attachment_handles[MAX_NUM_BASE_EXT_ATTACH_PTS] = {-1, -1};
	int num_att_pts = -1;
	BASE_EXT_TYPE base_ext_type = TWO_ATT_HORIZONTAL;

	static const int WIDTH;
	static const int HEIGHT;
};
struct attachment_t;
int create_base_ext(attachment_t& att, BASE_EXT_TYPE type);
void update_base_ext(base_extension_t& attachment);
void delete_base_ext(base_extension_t& attachment);

enum ATTACHMENT_TYPE : int {
	ATTMNT_NONE = 0,
	ATTMNT_GUN = 1 << 0,
	ATTMNT_BASE_EXT = 1 << 2,
};
OR_ENUM_DEFINITION(ATTACHMENT_TYPE)

enum class ATT_PLACEMENT {
	TOP = 0,
	RIGHT,
	LEFT,
	BOTTOM
};

typedef int att_pos_hash;
att_pos_hash hash_att_pos(float x, float y);

struct attachment_t {
	static std::unordered_map<att_pos_hash, int> overall_atts_placed_w_base_ext;
	int handle = -1;
	int transform_handle = -1;
	int quad_render_handle = -1;
	bool attached = false;
	ATT_PLACEMENT att_placement = ATT_PLACEMENT::TOP;

	ATTACHMENT_TYPE attachment_types = ATTMNT_NONE;

	static const int WIDTH;
	static const int HEIGHT;
};
int create_attachment(glm::vec2 pos, ATT_PLACEMENT att_placement, ATTACHMENT_TYPE attmt_types, base_t* base, base_extension_t* base_ext);
void update_attachment(attachment_t& attachment);
attachment_t* get_attachment(int handle);
void delete_attachment(int handle);
void delete_attachment(attachment_t& att);

enum class GUN_STATE {
	NONE = 0,
	ROTATING_TO_ENEMY,
	SHOOTING
};

struct gun_t {
	int handle = -1;

	int transform_handle = -1;
	int quad_render_handle = -1;
	int rb_handle = -1;

	int attachment_handle = -1;
	bool facing_left = false;

	GUN_STATE gun_state = GUN_STATE::NONE;

	float fire_rate = 0;
	time_count_t time_since_last_fire = 0;

	closest_entity_t closest_enemy;
	time_count_t last_retarget_time = 0;
	glm::vec2 dir_upon_target_fix = glm::vec2(0);

	static const int WIDTH;
	static const int HEIGHT;
	static const float RETARGET_ANIM_TIME;
	static const float MAX_DISTANCE_TO_ENEMY;

	static std::vector<int> enemy_died_handles;
};
void create_attached_gun(int attachment_handle, bool facing_left, float fire_rate);
void update_attached_gun(gun_t& gun);
void delete_attached_gun(gun_t& gun);

struct bullet_t {
	int handle = -1;

	glm::vec2 dir = glm::vec2(0);
	float speed = 0;
	time_count_t creation_time = 0;
	static const float ALIVE_TIME;

	int transform_handle = -1;
	int quad_render_handle = -1;
	int rb_handle = -1;

	static const int WIDTH;
	static const int HEIGHT;
};
void create_bullet(glm::vec2 start_pos, glm::vec2 move_dir, float speed);
void update_bullet(bullet_t& bullet);
void delete_bullet(bullet_t& bullet);

struct enemy_bullet_t {
	int handle = -1;

	int transform_handle = -1;
	int quad_render_handle = -1;
	int rb_handle = -1;

	glm::vec2 dir = glm::vec2(0);
	float speed = 0;
	time_count_t creation_time = 0;
	static const float ALIVE_TIME;

	static const int WIDTH;
	static const int HEIGHT;
};
void create_enemy_bullet(glm::vec2 pos, glm::vec2 dir, float speed);
void delete_enemy_bullet(enemy_bullet_t& enemy_bullet);

struct score_t {
	int enemies_left_to_kill = 10;
};
void update_score();

/**
 * @brief Update all gameobjects
*/
void gos_update();
void gos_update_delete_pass();
void delete_gos();