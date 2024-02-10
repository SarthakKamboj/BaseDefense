#pragma once

#include "glm/glm.hpp"

struct camera_t {
	glm::vec3 pos = glm::vec3(0);
	float rotation = 0;
	glm::vec2 drag_start_mouse_pos = glm::vec2(0);
	glm::vec2 drag_start_pos = glm::vec2(0);
	glm::vec2 cam_view_dimensions = glm::vec2(0);
	glm::vec2 cam_view_multiplers = glm::vec2(2);
	bool dragging = false;
	float scroll_speed = 40.f;
};

void init_camera();
glm::mat4 get_cam_view_matrix();
glm::vec2 mouse_to_world_pos();
glm::vec2 world_pos_to_screen(glm::vec2 world_pos);
glm::vec2 world_vec_to_screen_vec(glm::vec2 world_vec);
glm::vec2 normalized_screen_coord(glm::vec2 screen_coord);
bool within_screen(glm::vec2 screen_coords);
void update_camera();
