#pragma once

#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"

struct transform_t {
	glm::vec3 position = glm::vec3(0);
	glm::vec3 scale = glm::vec3(1);
	float rotation_deg = 0;
	float y_deg = 0;
    int handle = -1;

	float last_delta_x = 0;
	float last_delta_y = 0;
};

/// <summary>
/// Create a transform given a position, scale, and rotation
/// </summary>
/// <param name="position"></param>
/// <param name="scale"></param>
/// <param name="rot_deg">Rotation in degrees</param>
/// <returns>The handle associated with the created transform</returns>
int create_transform(glm::vec3 position, glm::vec3 scale, float rot_deg, float y_deg = 0.f);

/// <summary>
/// Creates the model matrix associated with a particular position, scale, and rotation
/// </summary>
/// <param name="transform">The world transform</param>
/// <returns>A 4x4 model (local to world) matrix</returns>
glm::mat4 get_model_matrix(transform_t& transform);

/// <summary>
/// Get the transform given the transform's handle
/// </summary>
/// <param name="transform_handle"></param>
/// <returns>A pointer to the transform for that handle, NULL if it doesn't exist</returns>
transform_t* get_transform(int transform_handle);

/**
 * @brief Delete a transform by handle
 * @param handle 
*/
void delete_transform(int handle);
