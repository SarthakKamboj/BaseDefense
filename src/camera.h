#pragma once

#include "glm/glm.hpp"

struct camera_t {
	glm::vec3 pos = glm::vec3(0, -10, 0);
	float rotation = 0;
};

glm::mat4 get_view_matrix(camera_t& cam);
void update(camera_t& cam);
