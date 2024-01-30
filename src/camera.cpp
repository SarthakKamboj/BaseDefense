#include "camera.h"

#include "glm/gtc/type_ptr.hpp"

#include "globals.h"
#include "utils/time.h"
#include "gameplay/preview_manager.h"

extern globals_t globals;
extern preview_state_t preview_state;

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

bool within_screen(glm::vec2 screen_coords) {
	return screen_coords.x >= 0 && screen_coords.x <= globals.window.window_width && screen_coords.y >= 0 && screen_coords.y <= globals.window.window_height;
}

void update_camera() {
	camera_t& cam = globals.camera;
	if (get_pressed(MIDDLE_MOUSE)) {
		cam.drag_start_mouse_pos = glm::vec2(globals.window.user_input.mouse_x, globals.window.user_input.mouse_y);
		cam.drag_start_pos = cam.pos;
		cam.dragging = true;
	} else if (get_released(MIDDLE_MOUSE)) {
		cam.dragging = false;
	}

	if (cam.dragging) {
		glm::vec2 cur_mouse = glm::vec2(globals.window.user_input.mouse_x, globals.window.user_input.mouse_y);
		float x_delta = cur_mouse.x - cam.drag_start_mouse_pos.x;
		cam.pos.x = cam.drag_start_pos.x - x_delta;
	}

	if (!get_down(CONTROLLER_Y) && !get_down(CONTROLLER_X)) {
		const float CAM_DRAG_SPEED = 300.f;
		float x_delta = globals.window.user_input.controller_x_axis * CAM_DRAG_SPEED * game::time_t::delta_time;
		cam.pos.x += x_delta;
	}

	if (globals.window.resized) {
		cam.cam_view_dimensions = glm::vec2(globals.window.window_width, globals.window.window_height);
		printf("globals.camera.cam_view_dimensions: %f, %f\n", globals.camera.cam_view_dimensions.x, globals.camera.cam_view_dimensions.x);
	}

	float mouse_scroll_delta = globals.window.user_input.mouse_scroll_wheel_delta_y;
	bool zooming_in_or_out = mouse_scroll_delta != 0 || get_down(CONTROLLER_LB) || get_down(CONTROLLER_RB);
	if (zooming_in_or_out) {
		float aspect_ratio = globals.window.window_height / globals.window.window_width;
		glm::vec2 delta_dim(0);
		if (is_controller_connected()) {
			float controller_scroll_speed = 300.f;
			glm::vec2 possible_delta_dim = glm::vec2(controller_scroll_speed, aspect_ratio * controller_scroll_speed) * glm::vec2(game::time_t::delta_time);
			if (get_down(CONTROLLER_LB)) {
				delta_dim = -possible_delta_dim;
			} else if (get_down(CONTROLLER_RB)) {
				delta_dim = possible_delta_dim;
			}
		} else {
			float mouse_scroll_speed = 40.f;
			delta_dim = glm::vec2(mouse_scroll_speed, aspect_ratio * mouse_scroll_speed);
			if (mouse_scroll_delta > 0) {
				delta_dim *= -1;
			} 
		}
		
		cam.cam_view_dimensions += delta_dim;
		printf("globals.camera.cam_view_dimensions: %f, %f\n", globals.camera.cam_view_dimensions.x, globals.camera.cam_view_dimensions.y);
	}
}
