#pragma once

#include "buffers.h"
#include "shader.h"

/// <summary>
/// A struct to store the VAO, EBO, VBO, and shader associated with a particular rendering object
/// </summary>
struct render_object_data {
	vao_t vao;
	ebo_t ebo;
	shader_t shader;
	vbo_t vbo;
};

/// <summary>
/// Draw an Opengl set of data
/// </summary>
/// <param name="data">OpenGL set of data</param>
void draw_obj(const render_object_data& data);
