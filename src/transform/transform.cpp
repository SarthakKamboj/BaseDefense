#include "transform.h"
#include <vector>

#include "constants.h"

static std::vector<transform_t> transforms;

int create_transform(glm::vec3 position, glm::vec3 scale, float rot_deg, float y_deg, int parent_transform_handle) {
    static int running_count = 0;
	transform_t transform;
    transform.local_position = position;
	transform.local_scale = scale;
    transform.local_rotation = glm::vec3(0, y_deg, rot_deg);

    transform.parent_transform_handle = parent_transform_handle;
    if (transform.parent_transform_handle == -1) {
        transform.global_position = transform.local_position;
        transform.global_scale = transform.local_scale;
        transform.global_rotation = transform.local_rotation;
    } else {
        transform_t* parent_transform = get_transform(parent_transform_handle);
        game_assert_msg(parent_transform, "parent transform of new transform doesn't exist");
        glm::mat4 cur_global_model = get_global_model_matrix(*parent_transform) * get_local_model_matrix(transform);
        transform.global_position = glm::vec3(cur_global_model[3].x, cur_global_model[3].y, cur_global_model[3].z);
    }
	// transform.global_position = position;
	// transform.global_scale = scale;
    // transform.global_rotation = glm::vec3(0, y_deg, rot_deg);
    transform.handle = running_count;
	transforms.push_back(transform);
    running_count++;
	return transform.handle;
}

glm::mat4 get_local_model_matrix(transform_t& transform) {
    glm::mat4 model = glm::mat4(1.0f);
    /* 
        we want to scale first, then rotate, then translate in the model space
        hence the calculation is T * R * S in linear algebra terms, which is
        why translation is done first in code, then rotation, then scale
    */
	model = glm::translate(model, transform.local_position);
	model = glm::rotate(model, glm::radians(transform.local_rotation.z), glm::vec3(0.f, 0.f, 1.f));
	model = glm::rotate(model, glm::radians(transform.local_rotation.y), glm::vec3(0.f, 1.f, .0f));
	model = glm::rotate(model, glm::radians(transform.local_rotation.x), glm::vec3(1.f, 0.f, .0f));
	const glm::vec3& scale = transform.local_scale;
	model = glm::scale(model, glm::vec3(scale.x, scale.y, 1.0f));
	return model;
}

glm::mat4 get_global_model_matrix(transform_t& transform) {
	glm::mat4 model = glm::mat4(1.0f);
    /* 
        we want to scale first, then rotate, then translate in the model space
        hence the calculation is T * R * S in linear algebra terms, which is
        why translation is done first in code, then rotation, then scale
    */
	model = glm::translate(model, transform.global_position);
	model = glm::rotate(model, glm::radians(transform.global_rotation.z), glm::vec3(0.f, 0.f, 1.f));
	model = glm::rotate(model, glm::radians(transform.global_rotation.y), glm::vec3(0.f, 1.f, .0f));
	model = glm::rotate(model, glm::radians(transform.global_rotation.x), glm::vec3(1.f, 0.f, .0f));
	const glm::vec3& scale = transform.global_scale;
	model = glm::scale(model, glm::vec3(scale.x, scale.y, 1.0f));
	return model;
}

glm::vec3 get_world_pos(transform_t* transform) {
    if (transform->parent_transform_handle == -1) {
        return transform->global_position;
    }
    transform_t* parent_transform = get_transform(transform->parent_transform_handle);
    game_assert_msg(parent_transform, "parent transform not found");
    return parent_transform->global_position + transform->global_position;
}

transform_t* get_transform(int transform_handle) {
    for (transform_t& transform : transforms) {
        if (transform.handle == transform_handle) {
            return &transform;
        }
    }
    return NULL;
}

void delete_transform(int handle) {
    if (handle == 0 || handle == 1) {
        int a = 5;
    }
    int i_to_remove = -1;
    for (int i = 0; i < transforms.size(); i++) {
        transform_t& transform = transforms[i];
        if (transform.handle == handle) {
            i_to_remove = i;
        }
    }
    if (i_to_remove != -1) {
        transforms.erase(transforms.begin() + i_to_remove);
    }
}
