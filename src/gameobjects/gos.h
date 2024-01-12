#pragma once

struct gun_base_t {
	int handle = -1;

	int quad_render_handle = -1;
	int rb_handle = -1;

	static const int WIDTH;
	static const int HEIGHT;
};
void create_gun_base();

/**
 * @brief Update all gameobjects
*/
void gos_update();