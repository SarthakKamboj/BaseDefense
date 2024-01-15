#pragma once

#include "glm/glm.hpp"

struct camera_t {
	// glm::vec3 internal_pos = glm::vec3(0);
	// glm::vec3 perceived_pos = glm::vec3(0);	
	// glm::vec3 internal_to_perceived_offset = glm::vec3(0);
	glm::vec3 pos = glm::vec3(0);
	float rotation = 0;
};

void init_camera();
glm::mat4 get_cam_view_matrix();
glm::vec2 mouse_to_world_pos();
void update_camera();
