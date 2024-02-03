#pragma once

#include "ui_elements.h"

void draw_background(widget_t& widget);
void draw_image_container(widget_t& widget);
void draw_text(const char* text, glm::vec2 starting_pos, int font_size, glm::vec3& color, int z_pos);



