#include "camera.h"

#include "glm/gtc/type_ptr.hpp"

#include "globals.h"

extern globals_t globals;

void init_camera() {
	globals.camera.rotation = 0;
}

glm::mat4 get_cam_view_matrix() {
	camera_t& cam = globals.camera;
	glm::mat4 view_mat(1.0f);
	view_mat = glm::translate(view_mat, -cam.pos);
	view_mat = glm::rotate(view_mat, glm::radians(-cam.rotation), glm::vec3(0.f, 0.f, 1.0f));
	return view_mat;
}

glm::vec2 mouse_to_world_pos() {
	glm::vec2 mouse = glm::vec2(globals.window.user_input.mouse_x, globals.window.user_input.mouse_y);
	glm::vec2 world_mouse = glm::vec2(globals.camera.pos.x, globals.camera.pos.y) + mouse;
	return world_mouse;
}

void update_camera() {}
