#include "transform.h"

#include <vector>

#include "glm/exponential.hpp"

#include "constants.h"

static std::vector<transform_t> transforms;

/*

// Q will be orthonormal
QR(in A, out Q, out R) {
    R[0][0] = len(A[0])
    Q[0] = normalize(A[0])

    R[1][0] = dot(A[1], Q[0])
    a1_perpen = A[1] - R[1][0] (Q[0])
    R[1][1] = len(a1_perpen)
    Q[1] = normalize(a1_perpen)

    R[2][0] = dot(A[2], Q[0])
    R[2][1] = dot(A[2], Q[1])
    a2_perpen = A[2] - R[2][0] (Q[0]) - R[2][1] (Q[1])
    R[2][2] = len(a2_perpen)
    Q[2] = normalize(a2_perpen)
}

Eigen(in A, out eigenvectors, out eigenvalues) {
    cur_Q = []
    cur_R = []
    X = A
    for (int i = 0; i < 30; i++) {
        cur_R = QR(X, cur_Q, cur_R)
        eigenvectors = eigenvector * cur_Q
        eigenvalues = cur_R * cur_Q
    }
}

SVD(in A, out U, out E, out V_t) {
    E^2 = []
    Eigen(A*At, U, E^2)    
    E = sqrtroot(E^2)
    V_t = E^-1 * U_t * A 
}

RotScaleTransform(in A, out rot, out scale, out transform) {
    transform = A[3][0:2];
    rot1 = []
    scale = []
    rot2 = []
    SVD(A, rot1, scale, rot2)
    rot = rot1 * rot2
}

*/

void qr(const glm::mat3& A, glm::mat3& Q, glm::mat3& R) {
    Q = glm::mat3(1.0f);
    R = glm::mat3(1.0f);

    R[0][0] = glm::length(A[0]);
    Q[0] = glm::normalize(A[0]);

    R[1][0] = glm::dot(A[1], Q[0]);
    auto& a1_perpen = A[1] - (R[1][0]*Q[0]);
    R[1][1] = glm::length(a1_perpen);
    Q[1] = glm::normalize(a1_perpen);

    R[2][0] = glm::dot(A[2], Q[0]);
    R[2][1] = glm::dot(A[2], Q[1]);
    auto& a2_perpen = A[2] - (R[2][0] * Q[0]) - (R[2][1] * Q[1]);
    R[2][2] = glm::length(a2_perpen);
    Q[2] = glm::normalize(a2_perpen);
}

void eigen(const glm::mat3& A, glm::mat3& eigenvectors, glm::mat3& eigenvalues) {
    glm::mat3 X = A;
    eigenvectors = glm::mat3(1.f);
    for (int i = 0; i < 30; i++) {
        glm::mat3 Q;
        glm::mat3 R;
        qr(X, Q, R);
        eigenvectors = eigenvectors * Q;
        X = R * Q;
    }
    eigenvalues = X;
}

void svd(const glm::mat3& A, glm::mat3& U, glm::mat3& E, glm::mat3& V_t) {
    glm::mat3 e2(1.f);
    eigen(A * glm::transpose(A), U, e2);

    E = glm::mat3(1.f);
    E[0][0] = sqrt(e2[0][0]);
    E[1][1] = sqrt(e2[1][1]);
    E[2][2] = sqrt(e2[2][2]);

    V_t = glm::inverse(E) * glm::transpose(U) * A;
}

void extract_translation_scale_rot(const glm::mat4& A, glm::vec3& translation, glm::vec3& scale, glm::vec3& rotation) {
    translation = glm::vec3(A[3][0], A[3][1], A[3][2]);

    glm::mat3 A_rot_scale(1.f);
    for (int col = 0; col < 3; col++) {
        A_rot_scale[col][0] = A[col][0];
        A_rot_scale[col][1] = A[col][1];
        A_rot_scale[col][2] = A[col][2];
    }
    glm::mat3 rot1(1.f);
    glm::mat3 scale_mat(1.f);
    glm::mat3 rot2(1.f);
    svd(A_rot_scale, rot1, scale_mat, rot2);
    scale = glm::vec3(scale_mat[0][0], scale_mat[1][1], scale_mat[2][2]);

    // https://www.geometrictools.com/Documentation/EulerAngles.pdf for further reference
    glm::mat3 rot = rot1 * rot2;
    float rot_x = 0, rot_y = 0, rot_z = 0;
    rot_y = asin(-rot[0][2]);

    if (glm::degrees(rot_y) == 90.f) {
        rot_z = 0.f;
        rot_x = atan2f(-rot[2][1], rot[1][1]);
    } else if (glm::degrees(rot_y) == -90.f) {
        rot_z = 0.f;
        rot_x = atan2f(-rot[2][1], rot[1][1]);
    } else {
        rot_x = atan2f(rot[1][2], rot[2][2]);
        rot_z = atan2f(rot[0][1], rot[0][0]);
    }
    rotation = glm::vec3(glm::degrees(rot_x), glm::degrees(rot_y), glm::degrees(rot_z));
}

int create_transform(glm::vec3 position, glm::vec3 scale, float rot_deg, float y_deg, int parent_transform_handle) {
    static int running_count = 0;
	transform_t transform;
    transform.handle = running_count++;
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

        parent_transform->child_transform_handles.push_back(transform.handle);

        glm::mat4 cur_global_model = get_global_model_matrix(*parent_transform) * get_local_model_matrix(transform);
        extract_translation_scale_rot(cur_global_model, transform.global_position, transform.global_scale, transform.global_rotation);
        
        // transform.global_position = glm::vec3(cur_global_model[3].x, cur_global_model[3].y, cur_global_model[3].z);

        // // need to do decomposition to get these
        // glm::mat4 rot_mat;
        // glm::mat4 scale_mat;
        // transform.global_scale = glm::vec3(scale_mat[0][0], scale_mat[1][1], scale_mat[2][2]);

        
    }
	// transform.global_position = position;
	// transform.global_scale = scale;
    // transform.global_rotation = glm::vec3(0, y_deg, rot_deg);
	transforms.push_back(transform);
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
    return transform->global_position;
    // if (transform->parent_transform_handle == -1) {
    //     return transform->global_position;
    // }
    // transform_t* parent_transform = get_transform(transform->parent_transform_handle);
    // game_assert_msg(parent_transform, "parent transform not found");
    // return parent_transform->global_position + transform->global_position;
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

void update_local_recursively(transform_t* transform) {
    if (transform->parent_transform_handle == -1) {
        transform->local_position = transform->global_position;
        transform->local_scale = transform->global_scale;
        transform->local_rotation = transform->global_rotation;
    } else {
        glm::mat4 cur_global = get_global_model_matrix(*transform);
        transform_t* parent_transform = get_transform(transform->parent_transform_handle);
        game_assert_msg(parent_transform, "parent transform of new transform doesn't exist");
        glm::mat4 parent_global = get_global_model_matrix(*parent_transform);
        glm::mat4 new_local = glm::inverse(parent_global)  * cur_global;
        extract_translation_scale_rot(new_local, transform->local_position, transform->local_scale, transform->local_rotation);
    }
    for (int i = 0; i < transform->child_transform_handles.size(); i++) {
        transform_t* t = get_transform(transform->child_transform_handles[i]);
        game_assert_msg(t, "transform for child handle not found");
        // update_local_recursively(t);
        update_global_recursively(t);
    }
}

void update_global_recursively(transform_t* transform) {
    if (transform->parent_transform_handle == -1) {
        transform->global_position = transform->local_position;
        transform->global_scale = transform->local_scale;
        transform->global_rotation = transform->local_rotation;
    } else {
        transform_t* parent_transform = get_transform(transform->parent_transform_handle);
        game_assert_msg(parent_transform, "parent transform of new transform doesn't exist");
        glm::mat4 cur_global_model = get_global_model_matrix(*parent_transform) * get_local_model_matrix(*transform);
        extract_translation_scale_rot(cur_global_model, transform->global_position, transform->global_scale, transform->global_rotation);
    }
    for (int i = 0; i < transform->child_transform_handles.size(); i++) {
        transform_t* t = get_transform(transform->child_transform_handles[i]);
        game_assert_msg(t, "transform for child handle not found");
        update_global_recursively(t);
    } 
}

void set_local_pos(transform_t* t, glm::vec3& pos) {
    t->local_position = pos;
    update_global_recursively(t);
}

void set_local_scale(transform_t* t, glm::vec3& scale) {
    t->local_scale = scale;
    update_global_recursively(t);
}

void set_local_rot(transform_t* t, glm::vec3& rot) {
    t->local_rotation = rot;
    update_global_recursively(t);
}

void set_global_pos(transform_t* t, glm::vec3& pos) {
    t->global_position = pos;
    update_local_recursively(t);
}

void set_global_scale(transform_t* t, glm::vec3& scale) {
    t->global_scale = scale;
    update_local_recursively(t);
}

void set_global_rot(transform_t* t, glm::vec3& rot) {
    t->global_rotation = rot;
    update_local_recursively(t);
}

void update_hierarchy_based_on_globals() {
    for (int i = 0; i < transforms.size(); i++) {
        if (transforms[i].parent_transform_handle == -1) {
            update_local_recursively(&transforms[i]);
        }
    }
}

void update_hierarchy_based_on_locals() {
    for (int i = 0; i < transforms.size(); i++) {
        if (transforms[i].parent_transform_handle == -1) {
            update_global_recursively(&transforms[i]);
        }
    }
}