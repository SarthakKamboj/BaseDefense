#pragma once

#include "glm/glm.hpp"

/// <summary>
/// Render the application
/// </summary>
/// <param name="app">The application struct with the application wide settings info</param>
void render();

glm::mat4 get_ortho_matrix(float width, float height);
bool detect_gfx_error();
void set_fill_mode();
void set_wireframe_mode();