#pragma once

#include "glm/glm.hpp"

/// <summary>
/// Represents a vertex on a 3D model with a position, color, and texture coordinate.
/// </summary>
struct vertex_t {
	glm::vec3 position = glm::vec3(0, 0, 0);
	glm::vec3 color = glm::vec3(0, 0, 0);
	glm::vec2 tex_coord = glm::vec2(0, 0);
};

/// <summary>
/// Creates a vertex with position, color, and texture coordinate
/// </summary>
/// <param name="position"></param>
/// <param name="color">RGB color. Ensure all values are normalized between 0 and 1.</param>
/// <param name="tex_coord">Texture coordinates.</param>
/// <returns></returns>
vertex_t create_vertex(glm::vec3 position, glm::vec3 color, glm::vec2 tex_coord);
