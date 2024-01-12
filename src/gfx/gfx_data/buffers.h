#pragma once

#include "glad/glad.h"

/// <summary>
/// An OpenGL Element Buffer Object
/// </summary>
struct ebo_t {
	GLuint id = 0;
	int num_indicies = -1;
};

/// <summary>
/// Creates an OpenGL EBO that will be mainly read-only after initial write
/// </summary>
/// <param name="indicies">An array of indicies</param>
/// <param name="size_of_buffer">The size of indicies in bytes (should be a multiple of the size of unsigned
/// int on the system, which is most likely 4 bytes)</param>
/// <returns>Created EBO</returns>
ebo_t create_ebo(const unsigned int* indicies, const int size_of_buffer);

/// <summary>
/// Creates an OpenGL EBO where the usage can be set manually
/// </summary>
/// <param name="indicies">An array of indicies</param>
/// <param name="size_of_buffer">The size of indicies in bytes (should be a multiple of the size of unsigned
/// int on the system, which is most likely 4 bytes)</param>
/// <param name="usage_pattern">The usage pattern of the EBO. Particularly useful to declare when a EBO
/// will be written to often.</param>
/// <returns>Created EBO</returns>
ebo_t create_ebo(const unsigned int* indicies, const int size_of_buffer, GLenum usage_pattern);

/// <summary>
/// Draw a particular EBO. Note, the EBO must be binded at this point for this function to have effect.
/// </summary>
/// <param name="ebo">EBO to draw</param>
void draw_ebo(const ebo_t& ebo);

/// <summary>
/// Bind an EBO for drawing or other forms of manipulation
/// </summary>
/// <param name="ebo">EBO to bind</param>
void bind_ebo(const ebo_t& ebo);

/// <summary>
/// Unbind the current EBO to ensure changes aren't made to it anymore or finished drawing
/// </summary>
void unbind_ebo();

/// <summary>
/// Delete an EBO
/// </summary>
/// <param name="ebo">EBO to delete</param>
void delete_ebo(const ebo_t& ebo);

/// <summary>
/// OpenGL Vertex Buffer Object
/// </summary>
struct vbo_t {
	GLuint id = 0;
};

/// <summary>
/// Creates an OpenGL VBO
/// </summary>
/// <param name="vertices">An array of the vertices</param>
/// <param name="data_size">The size of the array of vertices in bytes</param>
/// <returns>Create VBO</returns>
vbo_t create_vbo(const float* vertices, const int data_size);

vbo_t create_dyn_vbo(const int data_size);

/// <summary>
/// Bind a particular VBO to OpenGL state
/// </summary>
/// <param name="vbo">VBO to bind</param>
void bind_vbo(const vbo_t& vbo);

/// <summary>
/// Update the data associated with the VBO
/// </summary>
/// <param name="vbo">VBO to update</param>
/// <param name="vertices">An array of the new vertex data</param>
/// <param name="data_size">The size of the new vertex data array in bytes</param>
void update_vbo_data(const vbo_t& vbo, const float* vertices, const int data_size);

/// <summary>
/// Unbinds the current VBO
/// </summary>
void unbind_vbo();

/// <summary>
/// Deletes a VBO
/// </summary>
/// <param name="vbo">VBO to delete</param>
void delete_vbo(const vbo_t& vbo);

/// <summary>
/// OpenGL Vertex Array Object
/// </summary>
struct vao_t {
	GLuint id = 0;
};

/// <summary>
/// Creates a VAO
/// </summary>
/// <returns>Created VAO</returns>
vao_t create_vao();

/// <summary>
/// Binds a VAO to current OpenGL state
/// </summary>
/// <param name="vao">VAO to bind</param>
void bind_vao(const vao_t& vao);

/// <summary>
/// Unbinds current VAO
/// </summary>
void unbind_vao();

/// <summary>
/// Binds some particular set of data (vertex attribute) from the VBO data buffer to the VAO to be used by OpenGL shaders later on.
/// </summary>
/// <param name="vao">VAO that will house the VBO's vertex attribute data</param>
/// <param name="vbo">VBO with the data to be associated</param>
/// <param name="attrId">The vertex attribute index to be modified. Basically the slot within the 
/// VAO this VBO data will be stored.</param>
/// <param name="numValues">Number of values associated with the VBO vertex attribute</param>
/// <param name="dType">OpenGL type for the data to be stored</param>
/// <param name="stride">Number of bytes between consecutive vertex attributes</param>
/// <param name="offset">Offset in bytes of the first component of the first vertex attribute within the VBO buffer</param>
void vao_enable_attribute(vao_t& vao, const vbo_t& vbo, const int attrId, const int numValues, const int dType, const int stride, const int offset);

/// <summary>
/// Deletes a VAO
/// </summary>
/// <param name="vao">VAO to delete</param>
void delete_vao(const vao_t& vao);
