#pragma once

#include "utils/time.h"
#include "glm/glm.hpp"

struct base_t {
	int handle = -1;

	int transform_handle = -1;
	int quad_render_handle = -1;
	int rb_handle = -1;

	static const int WIDTH;
	static const int HEIGHT;
};
void create_base();

struct base_attachment_t {
	int handle = -1;

	int transform_handle = -1;
	int quad_render_handle = -1;
	int rb_handle = -1;

	int left_attach_pt_transform_handle = -1;
	int right_attach_pt_transform_handle = -1;

	bool left_attached = false;
	bool right_attached = false;

	static const int WIDTH;
	static const int HEIGHT;
};
int create_base_attachment();
void update_base_attachment(base_attachment_t& attachment);
base_attachment_t* get_base_attachment();

// TODO: review linear algebra transformations since gun will need to rotate around attachment point
// TODO: may need some sort of gameobject hierarchy
struct gun_t {
	int handle = -1;

	bool previewing = true;
	bool free = true;

	int transform_handle = -1;
	int quad_render_handle = -1;
	int rb_handle = -1;

	int base_attachment_handle = -1;
	bool left_attached = false;
	int attachment_transform_handle = -1;

	float fire_rate = 0;
	time_count_t time_since_last_fire = 0;

	static const int WIDTH;
	static const int HEIGHT;
};
void init_preview_gun();
void create_attached_gun(int base_attachment_handle, bool left_attached, float fire_rate);
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

/**
 * @brief Update all gameobjects
*/
void gos_update();