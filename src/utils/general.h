#pragma once

#include "glm/glm.hpp"

glm::vec3 create_color(float r, float g, float b);
void int_to_string(int val, char* buffer);
const char* get_file_extension(const char* path);