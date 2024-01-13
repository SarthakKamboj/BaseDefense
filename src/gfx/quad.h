#pragma once

#include "transform/transform.h"
#include "glm/glm.hpp"
#include "gfx_data/object_data.h"

/// <summary>
/// Represents the render settings for a quad in the game. Stores
/// information about the transform its associated with, the texture its associated with
/// and its influence, its color, width, height, and if it's in wireframe mode.
/// </summary>
struct quad_render_t {
    int handle = -1;
	int transform_handle = -1;
    int tex_handle = -1;
    float tex_influence = 0;
	glm::vec3 color = glm::vec3(0, 0, 0);
	float width = -1.f, height = -1.f;
	bool wireframe_mode = false;
	bool render = true;

    // each rectangle_rectangle_t is associated with one set of opengl vao, vbo, ebo
	static render_object_data obj_data;
};

void init_quad_data();

/// <summary>
/// Creates a quad to be render
/// </summary>
/// <param name="transform_handle">Transform handle associated with this render</param>
/// <param name="color">The base color</param>
/// <param name="width">In pixels</param>
/// <param name="height">In pixels</param>
/// <param name="wireframe">Whether wireframe mode or not, useful for debugging</param>
/// <param name="tex_influence">Influence of the texture if it exists</param>
/// <param name="tex_handle">Handle of the texture associated with the render quad</param>
/// <returns>The quad to be rendered</returns>
int create_quad_render(int transform_handle, glm::vec3& color, float width, float height, bool wireframe, float tex_influence, int tex_handle);

void set_quad_texture(int quad_handle, int tex_handle);
void set_quad_color(int quad_handle, glm::vec3& color);
void set_quad_width_height(int quad_handle, float width, float height);
quad_render_t* get_quad_render(int quad_handle);

/// <summary>
/// Draw a particular quad
/// </summary>
/// <param name="quad">Quad/recntagle to draw</param>
void draw_quad_render(const quad_render_t& quad);

void add_debug_pt(glm::vec3& pt);
void remove_debug_pt(glm::vec3& pt);
void draw_debug_pt(glm::vec3 pos);
void clear_debug_pts();

/// <summary>
/// Draw all quads in the game
/// </summary>
void draw_quad_renders();

/**
 * @brief Delete a quad render
 * @param quad_handle 
*/
void delete_quad_render(int quad_handle);