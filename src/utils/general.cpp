#include "general.h"

glm::vec3 create_color(float r, float g, float b) {
    return glm::vec3(r, g, b) / 255.f;
}

void int_to_string(int val, char* buffer) {
    sprintf(buffer, "%i", val);
}

const char* get_file_extension(const char* path) {
    return strrchr(path, '.') + 1;
}