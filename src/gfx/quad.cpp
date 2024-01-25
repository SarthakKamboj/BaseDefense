#include "quad.h"

#include <vector>
#include <iostream>

#include "renderer.h"
#include "transform/transform.h"
#include "utils/io.h"
#include "gfx_data/vertex.h"
#include "constants.h"
#include "globals.h"
#include "gfx_data/texture.h"

extern globals_t globals;

render_object_data quad_render_t::obj_data{};
std::vector<glm::vec3> debug_pts;

static std::vector<quad_render_t> quads;

/// <summary>
/// Initialize OpenGL data to render a quad to the screen. It creates the 4 vertices and stores it
/// in a VAO, VBO, and EBO. The rectangle vertex and fragment shaders are loaded where defaults are loaded.
/// The texture unit is defaulted to 0, the projection matrix is orthographic, and the view matrix
/// is the identity matrix.
/// </summary>
void init_quad_data() {

	render_object_data& data = quad_render_t::obj_data;

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
	// bind_vao(data.vao);
	vao_enable_attribute(data.vao, data.vbo, 0, 3, GL_FLOAT, sizeof(vertex_t), offsetof(vertex_t, position));
	vao_enable_attribute(data.vao, data.vbo, 1, 4, GL_FLOAT, sizeof(vertex_t), offsetof(vertex_t, color));
	vao_enable_attribute(data.vao, data.vbo, 2, 2, GL_FLOAT, sizeof(vertex_t), offsetof(vertex_t, tex_coord));
    vao_bind_ebo(data.vao, data.ebo);
	// bind_ebo(data.ebo);
	// unbind_vao();
	// unbind_ebo();

    // load in shader for these rectangle quads because the game is 2D, so everything is basically a solid color or a texture
	data.shader = create_shader("rectangle.vert", "rectangle.frag");
    // set projection matrix in the shader
	glm::mat4 projection = glm::ortho(0.0f, globals.window.window_width, 0.0f, globals.window.window_height);
	shader_set_mat4(data.shader, "projection", projection);
	shader_set_mat4(data.shader, "view", glm::mat4(1.0f));
	shader_set_int(data.shader, "tex", 0);

	if (detect_gfx_error()) {
		game_error_log("error loading the quad data");
	} else {
		game_info_log("successfully init quad data");
	}
}

int create_quad_render(int transform_handle, glm::vec3& color, float width, float height, bool wireframe, float tex_influence, int tex_handle) {
	game_assert(tex_influence >= 0.f);
	game_assert(tex_influence <= 1.f);
    static int running_count = 0;
	quad_render_t quad;
	quad.transform_handle = transform_handle;
	quad.width = width;
    quad.tex_influence = tex_influence;
    quad.tex_handle = tex_handle;
	quad.height = height;
	quad.color = color;
	quad.wireframe_mode = wireframe;
    // we scale internally because the original rectangle is 1x1 pixel wide and high
    quad.handle = running_count;
    quads.push_back(quad);
    running_count++;
	return quad.handle;
}

void set_quad_width_height(int quad_handle, float width, float height) {
	for (int i = 0; i < quads.size(); i++) {
        quad_render_t& quad = quads[i];
		if (quad.handle == quad_handle) {
			quad.width = width;
			quad.height = height;
			return;
		}
	}
}

quad_render_t* get_quad_render(int quad_handle) {
	for (int i = 0; i < quads.size(); i++) {
        quad_render_t& quad = quads[i];
		if (quad.handle == quad_handle) {
			return &quad;
		}
	}
	return NULL;
}

void set_quad_texture(int quad_handle, int tex_handle) {
	for (int i = 0; i < quads.size(); i++) {
        quad_render_t& quad = quads[i];
		if (quad.handle == quad_handle) {
			quad.tex_handle = tex_handle;
			return;
		}
	}
}

void set_quad_color(int quad_handle, glm::vec3& color) {
	for (int i = 0; i < quads.size(); i++) {
        quad_render_t& quad = quads[i];
		if (quad.handle == quad_handle) {
			quad.color = color;
			return;
		}
	}
}

void draw_quad_renders() {
	glm::mat4 view_mat = get_cam_view_matrix();
	shader_set_mat4(quad_render_t::obj_data.shader, "view", view_mat);
	glm::mat4 projection = glm::ortho(0.0f, globals.camera.cam_view_dimensions.x, 0.0f, globals.camera.cam_view_dimensions.y);
	shader_set_mat4(quad_render_t::obj_data.shader, "projection", projection);
    for (int i = 0; i < quads.size(); i++) {
        const quad_render_t& quad = quads[i];
		if (quad.render) {
			draw_quad_render(quad);
		}
	}
	for (glm::vec3& dp : debug_pts) {
		draw_debug_pt(dp);
	}
}


void add_debug_pt(glm::vec3& pt) {
	for (int i = 0; i < debug_pts.size(); i++) {
		if (pt == debug_pts[i]) {
			return;
		}
	}
	debug_pts.push_back(pt);
}

void remove_debug_pt(glm::vec3& pt) {
	for (int i = 0; i < debug_pts.size(); i++) {
		if (pt == debug_pts[i]) {
			debug_pts.erase(debug_pts.begin() + i);
			return;
		}
	}
}

void clear_debug_pts() {
	debug_pts.clear();
}

void draw_debug_pt(glm::vec3 pos) {
	transform_t cur_transform; 
	cur_transform.global_position = pos;
	cur_transform.global_scale *= 5.f;

	glm::mat4 model_matrix = get_global_model_matrix(cur_transform);
    // get model matrix and color and set them in the shader
	shader_set_mat4(quad_render_t::obj_data.shader, "model", model_matrix);
	shader_set_vec3(quad_render_t::obj_data.shader, "color", glm::vec3(1,0,0));
	shader_set_float(quad_render_t::obj_data.shader, "tex_influence", 0);
	// glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    set_fill_mode();
    // draw the rectangle render after setting all shader parameters
	draw_obj(quad_render_t::obj_data);	
}

void draw_quad_render(const quad_render_t& quad) {
    // get the transform for that rectangle render
    transform_t* transform_ptr = get_transform(quad.transform_handle);
    game_assert_msg(transform_ptr != NULL, "the transform for this quad doesn't exist");
	transform_t cur_transform = *transform_ptr;
    
	cur_transform.global_scale *= glm::vec3(quad.width, quad.height, 1.f);

	// glm::mat4 model_matrix(1.f);
	// if (transform_ptr->parent_transform_handle != -1) {
    // 	transform_t* parent_transform = get_transform(transform_ptr->parent_transform_handle);
    // 	game_assert_msg(parent_transform != NULL, "the parent transform for this quad doesn't exist");
	// 	model_matrix = get_global_model_matrix(*parent_transform);
	// }
	// model_matrix = model_matrix * get_global_model_matrix(cur_transform);
	glm::mat4 model_matrix = get_global_model_matrix(cur_transform);
    // get model matrix and color and set them in the shader
	shader_set_mat4(quad_render_t::obj_data.shader, "model", model_matrix);
	shader_set_vec3(quad_render_t::obj_data.shader, "color", quad.color);
#if _TESTING	
	shader_set_float(quad_render_t::obj_data.shader, "tex_influence", 0);
    unbind_texture();
#else
	shader_set_float(quad_render_t::obj_data.shader, "tex_influence", quad.tex_influence);
    bind_texture(quad.tex_handle);
#endif
	if (quad.wireframe_mode) {
		set_wireframe_mode();
	}
	else {
		// glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    	set_fill_mode();
	}
    // draw the rectangle render after setting all shader parameters
	draw_obj(quad_render_t::obj_data);
}

void delete_quad_render(int quad_handle) {
	int i_to_remove = -1;
	for (int i = 0; i < quads.size(); i++) {
		quad_render_t& quad = quads[i];
		if (quad.handle == quad_handle) {
			i_to_remove = i;
			break;
		}
	}
	if (i_to_remove != -1) {
		quads.erase(quads.begin() + i_to_remove);
	}
}

void delete_quad_renders() {
	quads.clear();
}