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

extern globals_t globals;

preview_state_t preview_state;
render_object_data preview_state_t::preview_render_data{};

const float preview_state_t::START_WIDTH = 200;
const float preview_state_t::START_HEIGHT = 200;

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
            preview_state.cur_mode = preview_state.cur_preview_selector_selected; 
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