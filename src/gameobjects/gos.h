#pragma once

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
base_attachment_t* get_base_attachment();

// TODO: review linear algebra transformations since gun will need to rotate around attachment point
// TODO: may need some sort of gameobject hierarchy
struct gun_t {
	int handle = -1;

	int transform_handle = -1;
	int quad_render_handle = -1;
	int rb_handle = -1;

	int base_attachment_handle = -1;
	bool left_attached = false;
	int attachment_transform_handle = -1;

	static const int WIDTH;
	static const int HEIGHT;
};
void create_gun(int base_attachment_handle, bool left_attached);
void update_gun(gun_t& gun);

/**
 * @brief Update all gameobjects
*/
void gos_update();