#include "camera.h"
#include "glm/gtc/type_ptr.hpp"

glm::mat4 get_view_matrix(camera_t& cam) {
	glm::mat4 view_mat(1.0f);
	view_mat = glm::translate(view_mat, -cam.pos);
	view_mat = glm::rotate(view_mat, glm::radians(-cam.rotation), glm::vec3(0.f, 0.f, 1.0f));
	return view_mat;
}

void update(camera_t& cam) {}
