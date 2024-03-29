#pragma once

#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"

#include <vector>

struct transform_t {
    int handle = -1;

	int parent_transform_handle = -1;
	std::vector<int> child_transform_handles;

	glm::vec2 global_position = glm::vec2(0);
	glm::vec2 global_scale = glm::vec2(1);
	glm::vec3 global_rotation = glm::vec3(0);

	glm::vec2 local_position = glm::vec2(0);
	glm::vec2 local_scale = glm::vec2(1);
	glm::vec3 local_rotation = glm::vec3(0);

	float z_pos = 0;
};

/// <summary>
/// Create a transform given a position, scale, and rotation
/// </summary>
/// <param name="position"></param>
/// <param name="scale"></param>
/// <param name="rot_deg">Rotation in degrees</param>
/// <returns>The handle associated with the created transform</returns>
int create_transform(glm::vec2 position, float z_pos, glm::vec2 scale, float rot_deg, float y_deg = 0.f, int parent_transform_handle = -1);

// TODO: need to think through cases where children get updated and not parents, how to handle that nicely

/// <summary>
/// Creates the model matrix associated with a particular position, scale, and rotation
/// </summary>
/// <param name="transform">The world transform</param>
/// <returns>A 4x4 model (local to world) matrix</returns>
glm::mat4 get_global_model_matrix(transform_t& transform, bool with_z = false);
glm::mat4 get_local_model_matrix(transform_t& transform);
void extract_translation_scale_rot(const glm::mat4& A, glm::vec3& translation, glm::vec3& scale, glm::vec3& rotation);
void svd(const glm::mat3& A, glm::mat3& U, glm::mat3& E, glm::mat3& V_t);
void eigen(const glm::mat3& A, glm::mat3& eigenvectors, glm::mat3& eigenvalues);
void qr(const glm::mat3& A, glm::mat3& Q, glm::mat3& R);

void update_global_recursively(transform_t* t);
void update_local_recursively(transform_t* t);

void update_hierarchy_based_on_globals();

void set_local_pos(transform_t* t, glm::vec2& pos);
void set_local_scale(transform_t* t, glm::vec2& scale);
void set_local_rot(transform_t* t, glm::vec3& rot);
void set_global_pos(transform_t* t, glm::vec2& pos);
void set_global_scale(transform_t* t, glm::vec2& scale);
void set_global_rot(transform_t* t, glm::vec3& rot);

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

void delete_transforms();