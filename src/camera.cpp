#include "camera.h"

#include "glm/gtc/type_ptr.hpp"

#include "globals.h"

extern globals_t globals;

void init_camera() {
	camera_t& cam = globals.camera;
	cam.internal_pos = glm::vec3(globals.window.window_width / 2, 0, 0);
	cam.internal_to_perceived_offset = glm::vec3(-globals.window.window_width / 2, 0, 0);
	cam.perceived_pos = cam.internal_pos +  cam.internal_to_perceived_offset;	
	cam.rotation = 0;
}

glm::mat4 get_cam_view_matrix() {
	camera_t& cam = globals.camera;
	glm::mat4 view_mat(1.0f);
	view_mat = glm::translate(view_mat, -cam.perceived_pos);
	view_mat = glm::rotate(view_mat, glm::radians(-cam.rotation), glm::vec3(0.f, 0.f, 1.0f));
	return view_mat;
}

void update_camera() {
	camera_t& cam = globals.camera;
	// eventually internal_pos and internal_to_perceived_offset won't just be negatives of each other
	cam.internal_pos = glm::vec3(globals.window.window_width / 2, 0, 0);
	cam.internal_to_perceived_offset = glm::vec3(-globals.window.window_width / 2, 0, 0);
	cam.perceived_pos = cam.internal_pos +  cam.internal_to_perceived_offset;	
}
