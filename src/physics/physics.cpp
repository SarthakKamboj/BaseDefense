#include "physics.h"
#include <vector>
#include "transform/transform.h"
#include <iostream>
#include "utils/time.h"

#include "constants.h"

#define TWO_DIM_RESOLVE 1
#define DIAG_METHOD_RESOLVE !TWO_DIM_RESOLVE

static std::vector<rigidbody_t> non_kin_rigidbodies;
static std::vector<rigidbody_t> kin_rigidbodies;

static std::vector<non_kin_w_kin_col_t> frame_non_kin_w_kin_cols;

static std::vector<kin_w_kin_col_t> frame_kin_w_kin_cols;

int create_rigidbody(int transform_handle, bool use_gravity, float collider_width, float collider_height, bool is_kinematic, PHYSICS_RB_LAYER rb_layer, bool detect_col, bool debug) {
    static int running_count = 0; 

	transform_t* transform = get_transform(transform_handle);
	game_assert_msg(transform, "could not get transform for this rigidbody");
	if (transform->parent_transform_handle != -1 && !is_kinematic) {
		game_assert_msg(false, "rigidbodies with transform parents must be kinematic");
	}

	rigidbody_t rigidbody;
	rigidbody.use_gravity = !is_kinematic && use_gravity;
	rigidbody.transform_handle = transform_handle;
	rigidbody.is_kinematic = is_kinematic;
    rigidbody.handle = running_count;
	rigidbody.rb_layer = rb_layer;
	rigidbody.debug = debug;
	rigidbody.detect_col = detect_col;
    running_count++;

	aabb_collider_t aabb_collider;
	aabb_collider.x = transform->global_position.x;
	aabb_collider.y = transform->global_position.y;
	aabb_collider.width = collider_width;
	aabb_collider.height = collider_height;

	// debug stuff
	aabb_collider.collider_debug_transform_handle = create_transform(glm::vec2(aabb_collider.x, aabb_collider.y), 0.f, glm::vec3(1.f), 0.f);
	aabb_collider.collider_debug_render_handle = -1;
	
	if (debug) {
		glm::vec3 collider_color(0.f, 1.f, 0.f);
		aabb_collider.collider_debug_render_handle = create_quad_render(aabb_collider.collider_debug_transform_handle, collider_color, collider_width, collider_height, true, 0, -1);
	}

	rigidbody.aabb_collider = aabb_collider;

	if (is_kinematic) {
		kin_rigidbodies.push_back(rigidbody);
	}
	else {
		non_kin_rigidbodies.push_back(rigidbody);
	}
	return rigidbody.handle;
}

void rigidbody_t::get_corners(glm::vec2 corners[4]) {
	glm::vec2& top_left = corners[CORNER::TOP_LEFT];
	glm::vec2& top_right = corners[CORNER::TOP_RIGHT];
	glm::vec2& bottom_right = corners[CORNER::BOTTOM_RIGHT];
	glm::vec2& bottom_left = corners[CORNER::BOTTOM_LEFT];
	
	glm::vec2 pos(aabb_collider.x, aabb_collider.y);

	float x_extent = abs(aabb_collider.width / 2);
	float y_extent = abs(aabb_collider.height / 2);
	top_left = glm::vec2(pos.x - x_extent, pos.y + y_extent);
	top_right = glm::vec2(pos.x + x_extent, pos.y + y_extent);
	bottom_left = glm::vec2(pos.x - x_extent, pos.y - y_extent);
	bottom_right = glm::vec2(pos.x + x_extent, pos.y - y_extent);
}

/**
 * @brief Get the normal vector to a line segment with two corners
 * @param corner1 The first corner of the line segment
 * @param corner2 The second corner of the line segment
*/
glm::vec2 get_normal(glm::vec2& corner1, glm::vec2& corner2) {
	glm::vec2 dir = corner2 - corner1;
	if (dir.y == 0) return glm::vec2(0, 1);
	return glm::vec2(1, -dir.x / dir.y);
}

/**
 * @brief Returns whether given the mins and maxs of projected polygons being compared for SAT collision detection, 
 * there is a overlap in these projections
 * @param min1 Minimum projection of the first polygon
 * @param max1 Maximum projection of the first polygon
 * @param min2 Minimum projection of the second polygon
 * @param max2 Maximum projection of the second polygon
 * @return Whether there was overlap
*/
bool sat_overlap(float min1, float max1, float min2, float max2) {
	bool no_overlap = (min1 > max2) || (min2 > max1);
	return !no_overlap;
}

/**
 * @brief Determines whether there is a collision being two rigidbodies using SAT
 * @param rb1 Rigidbody 1 for collision detection
 * @param rb2 Rigidbody 2 for collision detection
 * @return Whether there is a collision
*/
bool sat_detect_collision(rigidbody_t& rb1, rigidbody_t& rb2) {
	transform_t transform1_obj;
	transform1_obj.global_position.x = rb1.aabb_collider.x;
	transform1_obj.global_position.y = rb1.aabb_collider.y;
	transform_t* transform1 = &transform1_obj;

	transform_t transform2_obj;
	transform2_obj.global_position.x = rb2.aabb_collider.x;
	transform2_obj.global_position.y = rb2.aabb_collider.y;
	transform_t* transform2 = &transform2_obj;

	float grid1_x = floor(transform1->global_position.x / 40);
	float grid1_y = floor(transform1->global_position.y / 40);

	float grid2_x = floor(transform2->global_position.x / 40);
	float grid2_y = floor(transform2->global_position.y / 40);

	if (abs(grid2_x - grid1_x) >= 10 || abs(grid2_y - grid1_y) >= 10) return false;

	glm::vec2 rb1_corners[4];
	glm::vec2 rb2_corners[4];
	rb1.get_corners(rb1_corners);
	rb2.get_corners(rb2_corners);

	glm::vec2* rbs_corners[2] = {rb1_corners, rb2_corners};
	for (int j = 0; j < 1; j++) {
		glm::vec2* rb_corners = rbs_corners[j];
		for (int i = 0; i < 4; i++) {
			glm::vec2& corner1 = rb_corners[i];
			glm::vec2& corner2 = rb_corners[(i+1)%4];
			glm::vec2 normal = get_normal(corner1, corner2);

			float mins[2] = { std::numeric_limits<float>::max(), std::numeric_limits<float>::max() };
			float maxs[2] = { std::numeric_limits<float>::min(), std::numeric_limits<float>::min() };
			for (int rb_i = 0; rb_i < 2; rb_i++) {
				for (int corner_i = 0; corner_i < 4; corner_i++) {
					glm::vec2& corner = rbs_corners[rb_i][corner_i];
					float cast = corner.x * normal.x + corner.y * normal.y;
					mins[rb_i] = fmin(mins[rb_i], cast);
					maxs[rb_i] = fmax(maxs[rb_i], cast);
				}
			}

			if (!sat_overlap(mins[0], maxs[0], mins[1], maxs[1])) {
				return false;
			}
		}
	}
	return true;
}

/**
 * @brief Given a kinematic and non-kinematic rigidbody, determines the collision statuses of the 4 diagnals of the
 * non-kinematic rigidbody with the kinematic rigidbody and updates the col_info struct for a particular diagnal
 * if it has a collision closer to the center than what has already been calculated or defaulted
 * @param kin_rb Kinematic rb
 * @param non_kin_rb Non kinematic rb (such as the player)
 * @param total_col_info Running collision info of the 4 diagnals with this non kinematic rb
 * @param col_info Set collision info for this particular resolving, different from total_col_info because total_col_info is
 * a running record of collision for a non_kin rb with all kin_rbs, this is just for this particular non_kin and kin rb resolving
 * @return Whether there was any sort of collision using the diganals method and what the info was for it
*/
bool diagnals_method_col_info(rigidbody_t& kin_rb, rigidbody_t& non_kin_rb, collision_info_t& total_col_info, collision_info_t& cur_col_info) {
	glm::vec2 kin_corners[4];
	glm::vec2 non_kin_corners[4];

	bool detected_col = false;

	kin_rb.get_corners(kin_corners);
	non_kin_rb.get_corners(non_kin_corners);

	for (int non_kin_i = 0; non_kin_i < 4; non_kin_i++) {
		glm::vec2& non_kin_corner = non_kin_corners[non_kin_i];
		glm::vec2 non_kin_center = glm::vec2(non_kin_rb.aabb_collider.x, non_kin_rb.aabb_collider.y);
		float non_kin_slope = (non_kin_corner.y - non_kin_center.y) / (non_kin_corner.x - non_kin_center.x);

		diag_col_info_t& total_diag_col = total_col_info.diag_cols[non_kin_i];
		diag_col_info_t& cur_diag_col = cur_col_info.diag_cols[non_kin_i];

		for (int kin_i = 0; kin_i < 4; kin_i++) {
			glm::vec2& corner1 = kin_corners[kin_i];
			glm::vec2& corner2 = kin_corners[(kin_i+1)%4];

			// compared a vertical edge of the kin_rb
			if (corner1.x == corner2.x) {
				float y_point_on_non_kin_diag = non_kin_slope * (corner1.x - non_kin_center.x) + non_kin_center.y;
				glm::vec2 intersection_pt(corner1.x, y_point_on_non_kin_diag);
				float ratio_from_center_numerator = glm::pow(intersection_pt.x - non_kin_center.x, 2) + glm::pow(intersection_pt.y - non_kin_center.y, 2);
				float ratio_from_center_denom = glm::pow(non_kin_corner.x - non_kin_center.x, 2) + glm::pow(non_kin_corner.y - non_kin_center.y, 2);
				float dir = glm::dot(glm::normalize(intersection_pt - non_kin_center), glm::normalize(non_kin_corner - non_kin_center));
				float ratio_from_center = glm::sqrt(ratio_from_center_numerator / ratio_from_center_denom);
				if (dir >= 0 && ratio_from_center >= 0 && ratio_from_center <= 1.f && intersection_pt.y >= fmin(corner1.y, corner2.y) && intersection_pt.y <= fmax(corner1.y, corner2.y)) {
					// for this diagnal, this intersection is closer to the center
					if (ratio_from_center <= total_diag_col.ratio_from_center) {
						float move_ratio = 1 - ratio_from_center;
						transform_t* t_ptr = get_transform(non_kin_rb.transform_handle);
						game_assert(t_ptr != NULL);
						transform_t& transform = *t_ptr;
						glm::vec2 displacement = -(non_kin_corner - non_kin_center) * move_ratio;
						total_diag_col.dir = COL_DIR_HORIZONTAL;
						total_diag_col.ratio_from_center = ratio_from_center;
						total_diag_col.displacement = glm::vec2(displacement.x, 0.f);

						cur_diag_col.dir = COL_DIR_HORIZONTAL;
						cur_diag_col.ratio_from_center = ratio_from_center;
						cur_diag_col.displacement = displacement;

						detected_col = true;
					}
				}
			}

			// compared a horizontal edge of the kin_rb
			else if (corner1.y == corner2.y) {
				float x_point_on_non_kin_diag = ((corner1.y - non_kin_center.y) / non_kin_slope) + non_kin_center.x;
				glm::vec2 intersection_pt(x_point_on_non_kin_diag, corner1.y);
				float ratio_from_center_numerator = glm::pow(intersection_pt.x - non_kin_center.x, 2) + glm::pow(intersection_pt.y - non_kin_center.y, 2);
				float ratio_from_center_denom = glm::pow(non_kin_corner.x - non_kin_center.x, 2) + glm::pow(non_kin_corner.y - non_kin_center.y, 2);
				float dir = glm::dot(glm::normalize(intersection_pt - non_kin_center), glm::normalize(non_kin_corner - non_kin_center));
				float ratio_from_center = glm::sqrt(ratio_from_center_numerator / ratio_from_center_denom);
				if (dir >= 0 && ratio_from_center >= 0 && ratio_from_center <= 1.f && intersection_pt.x >= fmin(corner1.x, corner2.x) && intersection_pt.x <= fmax(corner1.x, corner2.x)) {
					// for this diagnal, this intersection is closer to the center
					if (ratio_from_center <= total_diag_col.ratio_from_center) {
						float move_ratio = 1 - ratio_from_center;
						transform_t* t_ptr = get_transform(non_kin_rb.transform_handle);
						game_assert(t_ptr != NULL);
						transform_t& transform = *t_ptr;
						glm::vec2 displacement = -(non_kin_corner - non_kin_center) * move_ratio;
						total_diag_col.dir = COL_DIR_VERTICAL;
						total_diag_col.ratio_from_center = ratio_from_center;
						total_diag_col.displacement = glm::vec2(0.f, displacement.y);

						cur_diag_col.dir = COL_DIR_VERTICAL;
						cur_diag_col.ratio_from_center = ratio_from_center;
						cur_diag_col.displacement = displacement;

						detected_col = true;
					}
				}
			}

		}
	}
	return detected_col;
}

void resolve(rigidbody_t& kin_rb, rigidbody_t& non_kin_rb, transform_t& non_kin_transform, col_dirs_t col_dirs) {
	if (col_dirs.right && non_kin_rb.vel.x > 0) {
		non_kin_rb.vel.x = 0;
		non_kin_transform.global_position.x = kin_rb.aabb_collider.x - (kin_rb.aabb_collider.width / 2) - (non_kin_rb.aabb_collider.width / 2);
	} else if (col_dirs.left && non_kin_rb.vel.x < 0) {
		non_kin_rb.vel.x = 0;
		non_kin_transform.global_position.x = kin_rb.aabb_collider.x + (kin_rb.aabb_collider.width / 2) + (non_kin_rb.aabb_collider.width / 2);
	}

	if (col_dirs.top && non_kin_rb.vel.y > 0) {
		non_kin_rb.vel.y = 0;
		non_kin_transform.global_position.y = kin_rb.aabb_collider.y - (kin_rb.aabb_collider.height / 2) - (non_kin_rb.aabb_collider.height / 2);				
	} else if (col_dirs.bottom && non_kin_rb.vel.y < 0) {
		non_kin_rb.vel.y = 0;
		non_kin_transform.global_position.y = kin_rb.aabb_collider.y + (kin_rb.aabb_collider.height / 2) + (non_kin_rb.aabb_collider.height / 2);				
	}
}

void update_rigidbodies() {	

	frame_non_kin_w_kin_cols.clear();
	frame_kin_w_kin_cols.clear();

	// update kinematic rigidbodies
	for (int i = 0; i < kin_rigidbodies.size(); i++) {
		rigidbody_t& rb1 = kin_rigidbodies[i];
#if 1
		for (int j = i+1; j < kin_rigidbodies.size(); j++) {
			rigidbody_t& rb2 = kin_rigidbodies[j];
			if (sat_detect_collision(rb1, rb2)) {
				kin_w_kin_col_t col_info;
				col_info.kin_handle1 = rb1.handle;
				col_info.kin_type1 = rb1.rb_layer;
				col_info.kin_handle2 = rb2.handle;
				col_info.kin_type2 = rb2.rb_layer;
				frame_kin_w_kin_cols.push_back(col_info);
			}
		}

#else
		transform_t* t = get_transform(rb1.transform_handle);
		game_assert(t);
		transform_t& transform = *t;

		if (rb.use_gravity) {
			rb.vel.y -= GRAVITY * delta_time;
			transform.global_position.y += rb.vel.y * delta_time;
		}

		rb.aabb_collider.x = transform.global_position.x;
		rb.aabb_collider.y = transform.global_position.y;

		if (rb.debug) {
			transform_t* debug_t = get_transform(rb.aabb_collider.collider_debug_transform_handle);
			game_assert(debug_t);
			transform_t& debug_transform = *debug_t;
			debug_transform.global_position.x = rb.aabb_collider.x;
			debug_transform.global_position.y = rb.aabb_collider.y;
		}
#endif
	}

	// update non-kinematic rigidbodies (which will also have no transform parents)
	for (rigidbody_t& non_kin_rb : non_kin_rigidbodies) {

		// update non kin rb position and velocity from gravity
		transform_t& transform = *get_transform(non_kin_rb.transform_handle);
		time_count_t delta_time = game::time_t::delta_time;
		
		if (non_kin_rb.use_gravity) {
			non_kin_rb.vel.y -= GRAVITY * delta_time;
		}

		transform.global_position.y += non_kin_rb.vel.y * delta_time;
		transform.global_position.x += non_kin_rb.vel.x * delta_time;

#if 0
		non_kin_rb.aabb_collider.x = transform.global_position.x;
		non_kin_rb.aabb_collider.y = transform.global_position.y;

		if (non_kin_rb.debug) {
			transform_t* debug_t = get_transform(non_kin_rb.aabb_collider.collider_debug_transform_handle);
			game_assert(debug_t);
			transform_t& debug_transform = *debug_t;
			debug_transform.global_position.x = non_kin_rb.aabb_collider.x;
			debug_transform.global_position.y = non_kin_rb.aabb_collider.y;
		}
#endif

		// if non kin rb is not detecting collision right now, just update positions and move on
		if (!non_kin_rb.detect_col) {
			continue;
		}

		collision_info_t total_col_info;

		for (int j = 0; j < kin_rigidbodies.size(); j++) {

			rigidbody_t& kin_rb = kin_rigidbodies[j];
			time_count_t delta_time = game::time_t::delta_time;

			if (!sat_detect_collision(non_kin_rb, kin_rb)) {
				continue;
			}

			collision_info_t total, cur;
			diagnals_method_col_info(kin_rb, non_kin_rb, total, cur);

			col_dirs_t col_dirs;
			for (int i = 0; i < 4; i++) {
				PHYSICS_COLLISION_DIR dir = cur.diag_cols[i].dir;
				if (dir == COL_DIR_NONE) continue;

				PHYSICS_RELATIVE_DIR rel_dir = REL_DIR_NONE;
				if (dir == COL_DIR_HORIZONTAL) {
					if (cur.diag_cols[i].displacement.x <= 0.f) {
						rel_dir = REL_DIR_RIGHT;
						col_dirs.right = true;
					}
					else {
						rel_dir = REL_DIR_LEFT;
						col_dirs.left = true;
					}
				}
				else {
					if (cur.diag_cols[i].displacement.y <= 0.f) {
						rel_dir = REL_DIR_TOP;
						col_dirs.top = true;
					}
					else {
						rel_dir = REL_DIR_BOTTOM;
						col_dirs.bottom = true;
					}
				} 
				non_kin_w_kin_col_t col_info;

				col_info.rel_dir = rel_dir;
				col_info.dir = dir;
				col_info.kin_handle = kin_rb.handle;
				col_info.kin_type = kin_rb.rb_layer;
				col_info.non_kin_type = non_kin_rb.rb_layer;

				frame_non_kin_w_kin_cols.push_back(col_info);
				break;

			}
	
			resolve(kin_rb, non_kin_rb, transform, col_dirs);

#if 0
			// update aabb positions
			non_kin_rb.aabb_collider.x = transform.global_position.x;
			non_kin_rb.aabb_collider.y = transform.global_position.y;

			// debugging collider
			transform_t* col_transform_ptr = get_transform(non_kin_rb.aabb_collider.collider_debug_transform_handle);
			game_assert(col_transform_ptr);
			transform_t& collider_debug_transform = *col_transform_ptr;
			collider_debug_transform.global_position.x = non_kin_rb.aabb_collider.x;
			collider_debug_transform.global_position.y = non_kin_rb.aabb_collider.y;
#endif
		}

	}

	update_hierarchy_based_on_globals();

	// update kinematic rigidbodies
	for (int i = 0; i < kin_rigidbodies.size() + non_kin_rigidbodies.size(); i++) {
		rigidbody_t* rb = NULL;
		if (i < kin_rigidbodies.size()) {
			rb = &kin_rigidbodies[i];
		} else {
			rb = &non_kin_rigidbodies[i];
		}
		transform_t* transform = get_transform(rb->transform_handle);
		game_assert(transform);

		rb->aabb_collider.x = transform->global_position.x;
		rb->aabb_collider.y = transform->global_position.y;

		if (rb->debug) {
			transform_t* debug_transform = get_transform(rb->aabb_collider.collider_debug_transform_handle);
			game_assert(debug_transform);
			debug_transform->global_position = transform->global_position;
			debug_transform->global_rotation = transform->global_rotation;
			debug_transform->global_scale = transform->global_scale;
			debug_transform->local_position = transform->local_position;
			debug_transform->local_scale = transform->local_scale;
			debug_transform->local_rotation = transform->local_rotation;
		}
	}
}

/**
 * @brief Get a rigidbody given a handle
 * @param rb_handle The handle of the rigidbody
 * @return Rigidbody with that handle
*/
rigidbody_t* get_rigidbody(int rb_handle) {
    for (rigidbody_t& rb : kin_rigidbodies) {
        if (rb.handle == rb_handle) {
            return &rb;
        }
    }
	for (rigidbody_t& rb : non_kin_rigidbodies) {
        if (rb.handle == rb_handle) {
            return &rb;
        }
    }
    return NULL;
}

void delete_rigidbody(int rb_handle) {
	int i_to_remove = -1;
	for (int i = 0; i < kin_rigidbodies.size(); i++) {
		if (kin_rigidbodies[i].handle == rb_handle) {
			i_to_remove = i;
			break;
		}
	}
	if (i_to_remove != -1) {
		if (kin_rigidbodies[i_to_remove].debug) {
			delete_quad_render(kin_rigidbodies[i_to_remove].aabb_collider.collider_debug_render_handle);
			delete_transform(kin_rigidbodies[i_to_remove].aabb_collider.collider_debug_transform_handle);
		}
		kin_rigidbodies.erase(kin_rigidbodies.begin() + i_to_remove);
		return;
	}

	for (int i = 0; i < non_kin_rigidbodies.size(); i++) {
		if (non_kin_rigidbodies[i].handle == rb_handle) {
			i_to_remove = i;
			break;
		}
	}

	if (i_to_remove == -1) return;
	if (non_kin_rigidbodies[i_to_remove].debug) {
		delete_quad_render(non_kin_rigidbodies[i_to_remove].aabb_collider.collider_debug_render_handle);
		delete_transform(non_kin_rigidbodies[i_to_remove].aabb_collider.collider_debug_transform_handle);
	}
	non_kin_rigidbodies.erase(non_kin_rigidbodies.begin() + i_to_remove);
}

std::vector<non_kin_w_kin_col_t> get_from_non_kin_w_kin_cols_w_non_kin(PHYSICS_RB_LAYER non_kin_type) {
	std::vector<non_kin_w_kin_col_t> col_infos;
	for (non_kin_w_kin_col_t& col_info : frame_non_kin_w_kin_cols) {
		if (col_info.non_kin_type == non_kin_type) {
			col_infos.push_back(col_info);
		}
	}
	return col_infos;
}

std::vector<non_kin_w_kin_col_t> get_from_non_kin_w_kin_cols_w_kin(int kin_handle, PHYSICS_RB_LAYER kin_type) {
	std::vector<non_kin_w_kin_col_t> col_infos;
	for (non_kin_w_kin_col_t& col_info : frame_non_kin_w_kin_cols) {
		if (col_info.kin_type == kin_type && col_info.kin_handle == kin_handle) {
			col_infos.push_back(col_info);
		}
	}
	return col_infos;
}

std::vector<kin_w_kin_col_t> get_from_kin_w_kin_cols(int kin_handle, PHYSICS_RB_LAYER kin_type) {
	std::vector<kin_w_kin_col_t> col_infos;
	for (kin_w_kin_col_t& col_info : frame_kin_w_kin_cols) {
		if ((col_info.kin_handle1 == kin_handle && col_info.kin_type1 == kin_type) || (col_info.kin_handle2 == kin_handle && col_info.kin_type2 == kin_type)) {
			col_infos.push_back(col_info);
		}
	}
	return col_infos;
}

void delete_rbs() {
	non_kin_rigidbodies.clear();
	kin_rigidbodies.clear();
	frame_non_kin_w_kin_cols.clear();
	frame_kin_w_kin_cols.clear();
}

void print_rb_layer(PHYSICS_RB_LAYER layer) {
	switch (layer) {
		case PHYS_NONE: {
			printf("phys none\n");
			break;
		}
		case PHYS_BASE: {
			printf("phys base \n");
			break;
		}
		case PHYS_BASE_EXT: {
			printf("phys base ext");
			break;
		}
		case PHYS_GUN: {
			printf("phys guns");
			break;
		}
		case PHYS_ENEMY: {
			printf("phys enemy");
			break;
		}
		case PHYS_BULLET: {
			printf("phys bullet");
			break;
		}
		case PHYS_ENEMY_BULLET: {
			printf("phys enemy bullet");
			break;
		}
		default: {
			printf("non recognized phys");
			break;
		}
	}
}