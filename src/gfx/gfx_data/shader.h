#pragma once

#include "glad/glad.h"
#include "glm/gtc/type_ptr.hpp"
#include <string>

/// <summary>
/// OpenGL Shader
/// </summary>
struct shader_t {
    bool valid = true;
	GLuint id = 0;
};

/// <summary>
/// Create an OpenGL shader given the vertex and fragment shader
/// </summary>
/// <param name="vert_path">The path to the GLSL vertex shader</param>
/// <param name="frag_path">The path to the GLSL fragment shader</param>
/// <returns>The shader</returns>
shader_t create_shader(const char* vert_file, const char* frag_file);

/// <summary>
/// Bind the shader by object
/// </summary>
/// <param name="shader"></param>
void bind_shader(const shader_t& shader);

/// <summary>
/// Unbind currently bound shader
/// </summary>
void unbind_shader();

/// <summary>
/// Set the 4x4 matrix in the shader by name
/// </summary>
/// <param name="shader"></param>
/// <param name="var_name">Name of the 4x4 matrix variable</param>
/// <param name="mat">4x4 matrix data</param>
void shader_set_mat4(shader_t& shader, const char* var_name, const glm::mat4& mat);

/// <summary>
/// Set int in shader by name
/// </summary>
/// <param name="shader"></param>
/// <param name="var_name"></param>
/// <param name="val">Int val</param>
void shader_set_int(shader_t& shader, const char* var_name, const int val);

/// <summary>
/// Set vec3 in shader by name
/// </summary>
/// <param name="shader"></param>
/// <param name="var_name"></param>
/// <param name="v">Vector3 data</param>
void shader_set_vec3(shader_t& shader, const char* var_name, const glm::vec3& v);

void shader_set_vec2(shader_t& shader, const char* var_name, const glm::vec2& v);

/// <summary>
/// Get the vector3 for a particular shader
/// </summary>
/// <param name="shader"></param>
/// <param name="var_name">Name of the Vector3 variable</param>
/// <returns></returns>
glm::vec3 shader_get_vec3(const shader_t& shader, const char* var_name);

/// <summary>
/// Set a float in a shader
/// </summary>
/// <param name="shader"></param>
/// <param name="var_name">Name of float variable</param>
/// <param name="val">Float value</param>
void shader_set_float(shader_t& shader, const char* var_name, const float val);
