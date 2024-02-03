#include "preview_manager.h"

#include "globals.h"
#include "transform/transform.h"
#include "gfx/quad.h"
#include "utils/general.h"
#include "gfx/gfx_data/texture.h"
#include "gfx/gfx_data/buffers.h"
#include "gfx/gfx_data/vertex.h"
#include "gfx/renderer.h"
#include "utils/io.h"
#include "camera.h"
#include "ui/ui.h"
#include "physics/physics.h"
#include "store.h"
#include "gos_globals.h"

#include <algorithm>

extern globals_t globals;
extern go_globals_t go_globals;
extern inventory_t inventory;
extern store_t store;

extern bool paused;

preview_state_t preview_state;
render_object_data preview_state_t::preview_render_data{};

const float preview_state_t::START_WIDTH = 200;
const float preview_state_t::START_HEIGHT = 200;

static glm::vec3 invalid_placement_color = create_color(144,66,71);
static glm::vec3 valid_placement_color = create_color(45,45,45);
static const float PREVIEW_MOVE_SPEED = 300.f;

void init_preview_mode() {

    char resources_path[256]{};
	get_resources_folder_path(resources_path);
	char image_path[512]{};
	sprintf(image_path, "%s\\%s\\selector\\circle.png", resources_path, ART_FOLDER);

    preview_state.circle_tex_handle = create_texture(image_path, 0);
    preview_state.transform_handle = create_transform(glm::vec3(0), glm::vec3(1), 0, 0, -1);
    preview_state.quad_render_handle = create_quad_render(preview_state.transform_handle, create_color(255,0,0), preview_state_t::START_WIDTH, preview_state_t::START_HEIGHT, false, 1.f, preview_state.circle_tex_handle);
    preview_state.window_rel_size = glm::vec2(preview_state_t::START_WIDTH, preview_state_t::START_HEIGHT) / glm::vec2(globals.window.window_width, globals.window.window_height);
    quad_render_t* q = get_quad_render(preview_state.quad_render_handle);
    game_assert_msg(q, "quad render not properly initizlied for preview");
    q->render = false;

    preview_state.cur_width = preview_state_t::START_WIDTH;
    preview_state.cur_height = preview_state_t::START_HEIGHT;
 
    render_object_data& data = preview_state_t::preview_render_data;

    if (!data.shader.valid) {
        // create the vertices of the rectangle
        vertex_t vertices[4];
        vertices[0] = create_vertex(glm::vec3(0.5f, 0.5f, 0.0f), glm::vec4(0,1,1,1), glm::vec2(1,1)); // top right
        vertices[1] = create_vertex(glm::vec3(0.5f, -0.5f, 0.0f), glm::vec4(0,0,1,1), glm::vec2(1,0)); // bottom right
        vertices[2] = create_vertex(glm::vec3(-0.5f, -0.5f, 0.0f), glm::vec4(0,1,0,1), glm::vec2(0,0)); // bottom left
        vertices[3] = create_vertex(glm::vec3(-0.5f, 0.5f, 0.0f), glm::vec4(1,0,0,1), glm::vec2(0,1)); // top left

        // setting the vertices in the vbo
        data.vbo = create_vbo((float*)vertices, sizeof(vertices));

        // creating the indicies for the rectangle
        unsigned int indices[] = {
            0, 1, 3,
            1, 2, 3
        };

        // set up ebo with indicies
        data.ebo = create_ebo(indices, sizeof(indices));

        // create vao and link the vbo/ebo to that vao
        data.vao = create_vao();
        vao_enable_attribute(data.vao, data.vbo, 0, 3, GL_FLOAT, sizeof(vertex_t), offsetof(vertex_t, position));
        vao_enable_attribute(data.vao, data.vbo, 1, 4, GL_FLOAT, sizeof(vertex_t), offsetof(vertex_t, color));
        vao_enable_attribute(data.vao, data.vbo, 2, 2, GL_FLOAT, sizeof(vertex_t), offsetof(vertex_t, tex_coord));
        vao_bind_ebo(data.vao, data.ebo);

        // load in shader for these rectangle quads because the game is 2D, so everything is basically a solid color or a texture
        data.shader = create_shader("selector.vert", "selector.frag");
        // set projection matrix in the shader
        // glm::mat4 projection = glm::ortho(0.0f, globals.window.window_width, 0.0f, globals.window.window_height);
        // shader_set_mat4(data.shader, "projection", projection);
        shader_set_mat4(data.shader, "view", glm::mat4(1.0f));
        shader_set_int(data.shader, "selector_image_tex", 0);

        if (detect_gfx_error()) {
            game_error_log("error loading the selector shader data");
        } else {
            game_info_log("successfully init selector shader data");
        }
    }

    init_preview_gun();
	init_base_ext_preview();
	init_preview_base();
}

void init_preview_base() {
	base_t& preview_base = preview_state.preview_base; 
    preview_base.handle = -1;
	preview_base.transform_handle = create_transform(glm::vec3(0, 0, go_globals.z_positions[PREVIEW_Z_POS_KEY]), glm::vec3(1), 0.f, 0.f);
	preview_base.quad_render_handle = create_quad_render(preview_base.transform_handle, create_color(60,90,30), base_t::WIDTH, base_t::HEIGHT, false, 0.f, -1);
	preview_base.rb_handle = create_rigidbody(preview_base.transform_handle, false, base_t::WIDTH, base_t::HEIGHT, true, PHYS_NONE, true, false);
}

void update_preview_base() {	
	base_t& preview_base = preview_state.preview_base; 
	quad_render_t* quad_render = get_quad_render(preview_base.quad_render_handle);
	game_assert_msg(quad_render, "quad render for base not found");	

	if (preview_state.cur_mode != PREVIEW_MODE::PREVIEW_BASE || store.open || paused) {
		quad_render->render = false;
		return;
	}

	transform_t* preview_transform = get_transform(preview_base.transform_handle);
	game_assert_msg(preview_transform, "could not find transform for preview base");

	if (!is_controller_connected()) {
		glm::vec2 mouse = mouse_to_world_pos();
		preview_transform->global_position = glm::vec3(mouse.x, base_t::HEIGHT * 0.5f, 0.f);
	} else {
		float cur_x = preview_transform->global_position.x;
		float delta_x = globals.window.user_input.controller_x_axis * PREVIEW_MOVE_SPEED * game::time_t::delta_time;
		preview_transform->global_position.x = cur_x + delta_x;
		preview_transform->global_position.y = base_t::HEIGHT * 0.5f;
	}

	if (inventory.num_bases == 0) {
		quad_render->color = invalid_placement_color;
	} else {
		quad_render->color = valid_placement_color;
	}

	bool released_preview_button = get_released(LEFT_MOUSE) || get_released(CONTROLLER_Y);
	if (released_preview_button && inventory.num_bases > 0) {
		glm::vec3& pos = preview_transform->global_position;
		create_base(glm::vec2(pos.x, pos.y));
		inventory.num_bases--;
	}

	bool preview_mode_on = get_down(LEFT_MOUSE) || get_down(CONTROLLER_Y);
	quad_render->render = preview_mode_on;

	update_hierarchy_based_on_globals();
}

void move_att_selection_left() {
	int possible_next = -1;
	int starting = preview_state.active_att_idx == -1 ? preview_state.sorted_att_infos.size() - 1 : preview_state.active_att_idx - 1;
	for (int i = starting; i >= 0; i--) {
		att_summary_info_t& summary = preview_state.sorted_att_infos[i];
		if (preview_state.cur_mode == PREVIEW_BASE_EXT && (summary.attachment_types & ATTMNT_BASE_EXT)) {
			possible_next = i;
			break;
		} else if (preview_state.cur_mode == PREVIEW_GUN && (summary.attachment_types & ATTMNT_GUN)) {
			possible_next = i;
			break;
		}
	}
	if (possible_next != -1) {
		preview_state.active_att_idx = possible_next;
		printf("attachment of handle %i found\n", preview_state.sorted_att_infos[possible_next].att_handle);
	}
	else {
		printf("no new attachment found\n");
	}
}

void move_att_selection_right() {
	int possible_next = -1;
	int starting = preview_state.active_att_idx == -1 ? 0 : preview_state.active_att_idx + 1;
	for (int i = starting; i < preview_state.sorted_att_infos.size(); i++) {
		att_summary_info_t& summary = preview_state.sorted_att_infos[i];
		if (preview_state.cur_mode == PREVIEW_BASE_EXT && (summary.attachment_types & ATTMNT_BASE_EXT)) {
			possible_next = i;
			break;
		} else if (preview_state.cur_mode == PREVIEW_GUN && (summary.attachment_types & ATTMNT_GUN)) {
			possible_next = i;
			break;
		}
	}
	if (possible_next != -1) {
		preview_state.active_att_idx = possible_next;
		printf("attachment of handle %i found\n", preview_state.sorted_att_infos[possible_next].att_handle);
	}
	else {
		printf("no new attachment found\n");
	}
}

void update_preview_mode() {
    bool selector_open = get_down(KEY_Z) || get_down(CONTROLLER_X);
    bool selector_released = get_released(KEY_Z) || get_released(CONTROLLER_X);

    if (selector_open || selector_released) {
        
        float ratio = preview_state_t::START_HEIGHT / preview_state_t::START_WIDTH;
        preview_state.cur_width = fmin(250.f, preview_state.window_rel_size.x * globals.camera.cam_view_dimensions.x);
        preview_state.cur_height = ratio * preview_state.cur_width;
        
        if (selector_open) {
            glm::vec2 mouse = mouse_to_world_pos();
            transform_t* transform = get_transform(preview_state.transform_handle);
            game_assert_msg(transform, "transform not found for preview state");

            if (!preview_state.preview_selector_open) {
                if (is_controller_connected()) {
                    transform->global_position.x = globals.window.window_width / 2.f;
                    transform->global_position.y = globals.window.window_height / 2.f;
                } else {
                    transform->global_position.x = mouse.x;
                    transform->global_position.y = mouse.y;
                }
                preview_state.preview_selector_open = true;
                update_hierarchy_based_on_globals();
            }

            glm::vec2 selector_pos(transform->global_position.x, transform->global_position.y);
            static glm::vec2 rel_pos;
            if (!is_controller_connected()){
                rel_pos = glm::normalize(mouse - selector_pos);
            }  else {
                glm::vec2 controller_axes(globals.window.user_input.controller_x_axis, globals.window.user_input.controller_y_axis);
                if (glm::distance(controller_axes, glm::vec2(0)) >= 0.5f) {
                    rel_pos = glm::normalize(controller_axes);
                }
            }

            float dot_w_vert = glm::dot(glm::vec2(0,1), rel_pos);

            float degrees_per_option = 360.f / NUM_PREVIEWABLE_ITEMS;

            if (dot_w_vert > glm::cos(glm::radians(degrees_per_option / 2.f))) {
                preview_state.cur_preview_selector_selected = PREVIEW_BASE;
            } else {
                if (glm::dot(glm::vec2(1,0), rel_pos) > 0) {
                    preview_state.cur_preview_selector_selected = PREVIEW_GUN;
                } else {
                    preview_state.cur_preview_selector_selected = PREVIEW_BASE_EXT;
                }
            }

        } else if (selector_released) {
            preview_state.preview_selector_open = false;
			if (preview_state.cur_mode != preview_state.cur_preview_selector_selected) {
            	preview_state.cur_mode = preview_state.cur_preview_selector_selected; 
				preview_state.active_att_idx = -1;
			}
        }
    }

    if (preview_state.cur_mode == PREVIEW_GUN) {
		set_ui_value(std::string("preview_mode"), std::string("preview gun mode"));
	} else if (preview_state.cur_mode == PREVIEW_BASE_EXT) {
		set_ui_value(std::string("preview_mode"), std::string("preview base ext mode"));
	} else if (preview_state.cur_mode == PREVIEW_BASE) {
		set_ui_value(std::string("preview_mode"), std::string("preview base mode"));
	} else {
		set_ui_value(std::string("preview_mode"), std::string("unrecognized preview mode"));
	}

	update_preview_base();	
	update_attachable_preview_item();
}

void render_preview_mode() {
    if (!preview_state.preview_selector_open) return;

    shader_t& shader = preview_state_t::preview_render_data.shader;

    glm::mat4 view_mat = get_cam_view_matrix();
	shader_set_mat4(shader, "view", view_mat);
	glm::mat4 projection = get_ortho_matrix(globals.window.window_width, globals.window.window_height);
	shader_set_mat4(shader, "projection", projection);

	quad_render_t* quad = get_quad_render(preview_state.quad_render_handle);
	game_assert_msg(quad, "could not find quad for preview_state");
    quad->width = preview_state.cur_width;
    quad->height = preview_state.cur_height;

    // get the transform for that rectangle render
    transform_t* transform_ptr = get_transform(preview_state.transform_handle);
    game_assert_msg(transform_ptr != NULL, "the transform for this quad doesn't exist");
	transform_t cur_transform = *transform_ptr;
	cur_transform.global_scale *= glm::vec3(quad->width, quad->height, 1.f);

	glm::mat4 model_matrix = get_global_model_matrix(cur_transform);
	shader_set_mat4(shader, "model", model_matrix);
	shader_set_vec3(shader, "color", quad->color);
    bind_texture(quad->tex_handle);
    set_fill_mode();

    glm::vec2 selection_pt(0);
    switch (preview_state.cur_preview_selector_selected) {
        case PREVIEW_BASE: {
            selection_pt = unit_circle_val(90);
            break;
        }
        case PREVIEW_BASE_EXT: {
            selection_pt = unit_circle_val(210);
            break;
        }
        case PREVIEW_GUN: {
            selection_pt = unit_circle_val(-30);
            break;
        }
    }
    
    shader_set_vec2(shader, "selection_point", selection_pt);

    // draw the rectangle render after setting all shader parameters
	draw_obj(preview_state_t::preview_render_data);
}

void init_base_ext_preview() {
    base_extension_t& preview_base_ext = preview_state.preview_base_ext;
	preview_base_ext.handle = -1;

	preview_base_ext.transform_handle = create_transform(glm::vec3(0, 0, go_globals.z_positions[PREVIEW_Z_POS_KEY]), glm::vec3(1), 0.f, 0.f);
	preview_base_ext.quad_render_handle = create_quad_render(preview_base_ext.transform_handle, create_color(145, 145, 145), base_extension_t::WIDTH, base_extension_t::HEIGHT, false, 0.f, -1);
	preview_base_ext.rb_handle = create_rigidbody(preview_base_ext.transform_handle, false, base_extension_t::WIDTH, base_extension_t::HEIGHT, true, PHYS_NONE, true, false);
}

void update_attachable_preview_item() {
	if (preview_state.cur_mode == PREVIEW_BASE || preview_state.cur_mode == PREVIEW_NONE) return;

	bool preview_down = get_down(LEFT_MOUSE) || get_down(CONTROLLER_Y);

	quad_render_t* preview_quad = NULL;
	if (preview_state.cur_mode == PREVIEW_BASE_EXT) {
		preview_quad = get_quad_render(preview_state.preview_base_ext.quad_render_handle);
	} else if (preview_state.cur_mode == PREVIEW_GUN) {
		preview_quad = get_quad_render(preview_state.preview_gun.quad_render_handle);
	}
	game_assert_msg(preview_quad, "quad render for preview attachment not found");

	if (store.open || paused) {
		preview_quad->render = false;
		return;
	}

	preview_quad->render = preview_down;

	if (!preview_down) {	
		return;
	}

	if (is_controller_connected() && get_down(CONTROLLER_Y)) {
		static bool centered = true;
		centered = centered || fabs(globals.window.user_input.controller_x_axis) < 0.1f;
		if (centered && (globals.window.user_input.controller_x_axis > .5f)) {
			move_att_selection_right();
			centered = false;
		} else if (centered && (globals.window.user_input.controller_x_axis < -.5f)) {
			move_att_selection_left();
			centered = false;
		}
	}

	if (inventory.num_base_exts == 0) {
		preview_quad->color = invalid_placement_color;
	} else {
		preview_quad->color = valid_placement_color;
	}	

	transform_t* transform = NULL;
	if (preview_state.cur_mode == PREVIEW_BASE_EXT) {
		transform = get_transform(preview_state.preview_base_ext.transform_handle);
	} else if (preview_state.cur_mode == PREVIEW_GUN) {
		transform = get_transform(preview_state.preview_gun.transform_handle);
	}
	game_assert_msg(transform, "transform of preview attachment not found");
	if (is_controller_connected()) {
		if (preview_state.active_att_idx != -1) {
			att_summary_info_t& summary = preview_state.sorted_att_infos[preview_state.active_att_idx];
			transform_t* att_transform = get_transform(summary.att_transform_handle);
			game_assert_msg(att_transform, "att transform not found");

			transform->global_position.x = att_transform->global_position.x;
			transform->global_position.y = att_transform->global_position.y;
		}
	} else {
		glm::vec2 mouse = mouse_to_world_pos();
		// want to maintain z value
		transform->global_position.x = mouse.x;
		transform->global_position.y = mouse.y;
	}
}

void init_preview_gun() {
    gun_t& preview_gun = preview_state.preview_gun;
	preview_gun.handle = -1;

	preview_gun.attachment_handle = -1;

	preview_gun.transform_handle = create_transform(glm::vec3(0, 0, go_globals.z_positions[PREVIEW_Z_POS_KEY]), glm::vec3(1), 0.f, 0.f, -1);
	preview_gun.quad_render_handle = create_quad_render(preview_gun.transform_handle, valid_placement_color, gun_t::WIDTH, gun_t::HEIGHT, false, 0.f, -1);
	preview_gun.rb_handle = create_rigidbody(preview_gun.transform_handle, false, gun_t::WIDTH, gun_t::HEIGHT, true, PHYS_NONE, false, false);
}

struct att_summary_info_less_than {
	inline bool operator() (const att_summary_info_t& summary1, const att_summary_info_t& summary2) {
		return summary1.x_pos < summary2.x_pos;
	}
};

void add_attachment_to_preview_manager(attachment_t& att) {
	att_summary_info_t summary;	
	summary.att_handle = att.handle;
	summary.att_transform_handle = att.transform_handle;
	summary.attachment_types = att.attachment_types;
	transform_t* att_transform = get_transform(att.transform_handle);
	game_assert_msg(att_transform, "transform for att not found");
	summary.x_pos = att_transform->global_position.x;
	preview_state.sorted_att_infos.push_back(summary);

	std::sort(preview_state.sorted_att_infos.begin(), preview_state.sorted_att_infos.end(), att_summary_info_less_than());
	printf("added att handle %i at x pos %f\n", summary.att_handle, summary.x_pos);
}

void delete_attachment_from_preview_manager(int att_handle) {
	auto& arr = preview_state.sorted_att_infos;
	for (int i = 0; i < arr.size(); i++) {
		att_summary_info_t& summary = arr[i];
		if (summary.att_handle == att_handle) {
			if (i == preview_state.active_att_idx) {
				preview_state.active_att_idx = -1;
				printf("removed att handle %i at x pos %f\n", summary.att_handle, summary.x_pos);
			} else if (i < preview_state.active_att_idx) {
				preview_state.active_att_idx--;
				printf("removed att handle %i at x pos %f\n", summary.att_handle, summary.x_pos);
			}
			arr.erase(arr.begin() + i);
		}
	}
}