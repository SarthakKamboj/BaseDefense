#include "shader.h"

#include "utils/io.h"
#include "constants.h"

#include "glad/glad.h"

#include <iostream>

// SHADER
shader_t create_shader(const char* vert_file, const char* frag_file) {
	shader_t shader;
	shader.valid = true;

	GLuint vert_shader = glCreateShader(GL_VERTEX_SHADER);
	GLuint frag_shader = glCreateShader(GL_FRAGMENT_SHADER);

	int success;
	char info_log[512];

	char resource_path[256]{};
	get_resources_folder_path(resource_path);

	char vert_path[256]{};
	sprintf(vert_path, "%s\\%s\\%s", resource_path, SHADERS_FOLDER, vert_file);
	char frag_path[256]{};
	sprintf(frag_path, "%s\\%s\\%s", resource_path, SHADERS_FOLDER, frag_file);

	std::cout << vert_path << std::endl;
	std::cout << frag_path << std::endl;

	std::string vert_code_str = get_file_contents(vert_path);
	const char* vert_shader_source = vert_code_str.c_str();
	glShaderSource(vert_shader, 1, &vert_shader_source, NULL);
	glCompileShader(vert_shader);
	glGetShaderiv(vert_shader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(vert_shader, 512, NULL, info_log);
		std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" + std::string(info_log) << std::endl;
		throw std::runtime_error("ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" + std::string(info_log));
	}

	std::string frag_code_str = get_file_contents(frag_path);
	const char* frag_shader_source = frag_code_str.c_str();
	glShaderSource(frag_shader, 1, &frag_shader_source, NULL);
	glCompileShader(frag_shader);
	glGetShaderiv(frag_shader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(frag_shader, 512, NULL, info_log);
		std::cout << "error::shader::fragment::compilation_failed\n" + std::string(info_log) << std::endl;
		throw std::runtime_error("error::shader::fragment::compilation_failed\n" + std::string(info_log));
	}

	shader.id = glCreateProgram();
	glAttachShader(shader.id, vert_shader);
	glAttachShader(shader.id, frag_shader);
	glLinkProgram(shader.id);
	glGetProgramiv(shader.id, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(shader.id, 512, NULL, info_log);
		throw std::runtime_error("ERROR::SHADER::PROGRAM::LINKING_FAILED\n" + std::string(info_log));
	}

	return shader;
}

void bind_shader(const shader_t& shader) {
	glUseProgram(shader.id);
}

void unbind_shader() {
	glUseProgram(0);
}

void shader_set_mat4(shader_t& shader, const char* var_name, const glm::mat4& mat) {
	glUseProgram(shader.id);
	GLint loc = glGetUniformLocation(shader.id, var_name);
    if (loc == -1) {
        std::cout << var_name << " does not exist in shader " << shader.id << std::endl;
    }
	glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(mat));
}

void shader_set_int(shader_t& shader, const char* var_name, const int val) {
	glUseProgram(shader.id);
	GLint loc = glGetUniformLocation(shader.id, var_name);
    if (loc == -1) {
        std::cout << var_name << " does not exist in shader " << shader.id << std::endl;
    }
	glUniform1i(loc, val);
}

void shader_set_vec3(shader_t& shader, const char* var_name, const glm::vec3& v) {
	glUseProgram(shader.id);
	GLint loc = glGetUniformLocation(shader.id, var_name);
    if (loc == -1) {
        std::cout << var_name << " does not exist in shader " << shader.id << std::endl;
    }
	glUniform3fv(loc, 1, glm::value_ptr(v));
}

void shader_set_vec2(shader_t& shader, const char* var_name, const glm::vec2& v) {
	glUseProgram(shader.id);
	GLint loc = glGetUniformLocation(shader.id, var_name);
    if (loc == -1) {
        std::cout << var_name << " does not exist in shader " << shader.id << std::endl;
    }
	glUniform2fv(loc, 1, glm::value_ptr(v));
}

void shader_set_float(shader_t& shader, const char* var_name, const float val) {
	glUseProgram(shader.id);
	GLint loc = glGetUniformLocation(shader.id, var_name);
    if (loc == -1) {
        std::cout << var_name << " does not exist in shader " << shader.id << std::endl;
    }
	glUniform1f(loc, val);
}

glm::vec3 shader_get_vec3(const shader_t& shader, const char* var_name) {
	glm::vec3 v;
	GLint loc = glGetUniformLocation(shader.id, var_name);
    if (loc == -1) {
        std::cout << var_name << " does not exist in shader " << shader.id << std::endl;
    }
	glGetUniformfv(shader.id, loc, &v[0]);
	return v;
}
