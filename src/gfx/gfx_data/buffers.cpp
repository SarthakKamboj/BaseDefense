#include "buffers.h"
#include <array>

// VBO
vbo_t create_vbo(const float* vertices, const int data_size) {
	vbo_t vbo;
	glGenBuffers(1, &vbo.id);
	glBindBuffer(GL_ARRAY_BUFFER, vbo.id);
	glBufferData(GL_ARRAY_BUFFER, data_size, vertices, GL_STATIC_DRAW);
	return vbo;
}

vbo_t create_dyn_vbo(const int data_size) {
	vbo_t vbo;
	glGenBuffers(1, &vbo.id);
	glBindBuffer(GL_ARRAY_BUFFER, vbo.id);
	glBufferData(GL_ARRAY_BUFFER, data_size, NULL, GL_DYNAMIC_DRAW);
	return vbo;
}

void update_vbo_data(const vbo_t& vbo, const float* vertices, const int data_size) {
	glBindBuffer(GL_ARRAY_BUFFER, vbo.id);
	glBufferSubData(GL_ARRAY_BUFFER, 0, data_size, vertices);
}

void bind_vbo(const vbo_t& vbo) {
	glBindBuffer(GL_ARRAY_BUFFER, vbo.id);
}

void unbind_vbo() {
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void delete_vbo(const vbo_t& vbo) {
	glDeleteBuffers(1, &vbo.id);
}

// EBO
ebo_t create_ebo(const unsigned int* indicies, const int size_of_buffer) {
	ebo_t ebo;
	ebo.num_indicies = size_of_buffer / sizeof(indicies[0]);
	glGenBuffers(1, &ebo.id);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo.id);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, size_of_buffer, indicies, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	return ebo;
}

ebo_t create_ebo(const unsigned int* indicies, const int size_of_buffer, GLenum usage_pattern) {
	ebo_t ebo;
	ebo.num_indicies = size_of_buffer / sizeof(indicies[0]);
	glGenBuffers(1, &ebo.id);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo.id);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, size_of_buffer, indicies, usage_pattern);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	return ebo;
}

void draw_ebo(const ebo_t& ebo) {
	glDrawElements(GL_TRIANGLES, ebo.num_indicies, GL_UNSIGNED_INT, 0);
}

void bind_ebo(const ebo_t& ebo) {
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo.id);
}

void unbind_ebo() {
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void delete_ebo(const ebo_t& ebo) {
	glDeleteBuffers(1, &ebo.id);
}

// VAO
vao_t create_vao() {
	vao_t vao;
	glGenVertexArrays(1, &vao.id);
	return vao;
}

void bind_vao(const vao_t& vao) {
	glBindVertexArray(vao.id);
}

void unbind_vao() {
	glBindVertexArray(0);
}

void vao_enable_attribute(vao_t& vao, const vbo_t& vbo, const int attrId, const int numValues, const int dType, const int stride, const int offset) {
	bind_vao(vao);
	bind_vbo(vbo);
	glVertexAttribPointer(attrId, numValues, dType, GL_FALSE, stride, reinterpret_cast<void*>(offset));
	glEnableVertexAttribArray(attrId);
}

void delete_vao(const vao_t& vao) {
	glDeleteVertexArrays(1, &vao.id);
}