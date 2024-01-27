#include "camera.h"

#include "glm/gtc/type_ptr.hpp"

#include "globals.h"
#include "utils/time.h"

extern globals_t globals;

void init_camera() {
	globals.camera.rotation = 0;
	globals.camera.cam_view_dimensions = glm::vec2(globals.window.window_width, globals.window.window_height);
	printf("globals.camera.cam_view_dimensions: %f, %f\n", globals.camera.cam_view_dimensions.x, globals.camera.cam_view_dimensions.x);
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
	glm::vec2 mouse_normalized = mouse / glm::vec2(globals.window.window_width, globals.window.window_height);
	glm::vec2 game_scroll_relative_pos = mouse_normalized * globals.camera.cam_view_dimensions;
	glm::vec2 world_mouse = glm::vec2(globals.camera.pos.x, globals.camera.pos.y) + game_scroll_relative_pos;
	return world_mouse;
}

glm::vec2 world_pos_to_screen(glm::vec2 world_pos) {
	glm::vec2 cam_rel_world_pos = world_pos - glm::vec2(globals.camera.pos.x, globals.camera.pos.y);
	glm::vec2 normalized_screen_pos = cam_rel_world_pos / globals.camera.cam_view_dimensions;
	return normalized_screen_pos * glm::vec2(globals.window.window_width, globals.window.window_height);
}

glm::vec2 world_vec_to_screen_vec(glm::vec2 world_vec) {
	return (world_vec / globals.camera.cam_view_dimensions) * glm::vec2(globals.window.window_width, globals.window.window_height);
}

void update_camera() {
	camera_t& cam = globals.camera;
	if (globals.window.user_input.middle_mouse_down_click) {
		cam.drag_start_mouse_pos = glm::vec2(globals.window.user_input.mouse_x, globals.window.user_input.mouse_y);
		cam.drag_start_pos = cam.pos;
		cam.dragging = true;
	} else if (globals.window.user_input.middle_mouse_release) {
		cam.dragging = false;
	}

	if (cam.dragging) {
		glm::vec2 cur_mouse = glm::vec2(globals.window.user_input.mouse_x, globals.window.user_input.mouse_y);
		float x_delta = cur_mouse.x - cam.drag_start_mouse_pos.x;
		cam.pos.x = cam.drag_start_pos.x - x_delta;
	}

	if (globals.window.resized) {
		cam.cam_view_dimensions = glm::vec2(globals.window.window_width, globals.window.window_height);
		printf("globals.camera.cam_view_dimensions: %f, %f\n", globals.camera.cam_view_dimensions.x, globals.camera.cam_view_dimensions.x);
	}

	if (globals.window.user_input.mouse_scroll_wheel_delta_y != 0) {
		int multiplier = globals.window.user_input.mouse_scroll_wheel_delta_y > 0 ? -1 : 1;
		float aspect_ratio = globals.window.window_height / globals.window.window_width;
		cam.cam_view_dimensions.x += multiplier * cam.scroll_speed;
		cam.cam_view_dimensions.y += multiplier * aspect_ratio * cam.scroll_speed;
		printf("globals.camera.cam_view_dimensions: %f, %f\n", globals.camera.cam_view_dimensions.x, globals.camera.cam_view_dimensions.y);
	}
}
