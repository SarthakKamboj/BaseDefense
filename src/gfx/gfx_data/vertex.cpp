#include "vertex.h"

vertex_t create_vertex(glm::vec3 position, glm::vec3 color, glm::vec2 tex_coord) {
	vertex_t vertex;
	vertex.position = position;
	vertex.color = color;
	vertex.tex_coord = tex_coord;
	return vertex;
}