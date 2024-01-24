#pragma once

#include "glm/glm.hpp"

glm::vec3 create_color(float r, float g, float b);
glm::vec4 create_color(float r, float g, float b, float a);
void int_to_string(int val, char* buffer);
const char* get_file_extension(const char* path);
glm::vec2 unit_circle_val(float degrees);
glm::vec2 lerp(glm::vec2 start, glm::vec2 end, float t);