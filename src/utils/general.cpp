#include "general.h"

glm::vec3 create_color(float r, float g, float b) {
    return glm::vec3(r, g, b) / 255.f;
}

glm::vec4 create_color(float r, float g, float b, float a) {
    return glm::vec4(r / 255.f, g / 255.f, b / 255.f, a);
}

void int_to_string(int val, char* buffer) {
    sprintf(buffer, "%i", val);
}

const char* get_file_extension(const char* path) {
    return strrchr(path, '.') + 1;
}

glm::vec2 unit_circle_val(float degrees) {
    return glm::vec2(glm::cos(glm::radians(degrees)), glm::sin(glm::radians(degrees)));
}

glm::vec2 lerp(glm::vec2 start, glm::vec2 end, float t) {
    return (1 - t) * start + (end * t);
}

bool in_between(float val, float min, float max) {
    return val >= min && val <= max;
}

float angle_of_vec(glm::vec2 v) {
    v = glm::normalize(v);
    float initial = glm::degrees(acos(v.x));
    if (glm::dot(glm::vec2(0, 1), v) < 0) {
        return 360.f - initial;
    }
    return initial;
}

float angle_between_vec2(glm::vec2 v, glm::vec2 reference) {
    float cos_z = glm::dot(glm::normalize(v), glm::normalize(reference));
    float z = glm::degrees(acos(cos_z));
    glm::vec2 perpen_ref = unit_circle_val(angle_of_vec(reference) + 90.f);
    if (glm::dot(perpen_ref, v) < 0) {
        return 360.f - z;
    }
    return z;
}