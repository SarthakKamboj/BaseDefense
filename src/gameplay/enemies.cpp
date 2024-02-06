#include "enemies.h"

#include "transform/transform.h"
#include "gfx/quad.h"
#include "physics/physics.h"
#include "gos_globals.h"
#include "constants.h"
#include "store.h"
#include "camera.h"
#include "ui/ui.h"
#include "ui/ui_elements.h"
#include "utils/general.h"
#include "globals.h"

extern go_globals_t go_globals;
extern globals_t globals;

std::vector<int> enemy_t::deleted_base_handles;

static int enemy_cnt = 0;

const int enemy_t::GROUND_HEIGHT = 120;
const int enemy_t::GROUND_WIDTH = 40;
const int enemy_t::AIR_HEIGHT = 40;
const int enemy_t::AIR_WIDTH = 40;
void create_enemy(glm::vec2 pos, MOVE_DIR dir, float speed) {
	enemy_t enemy;
	enemy.handle = enemy_cnt++;
	enemy.dir = dir;
	enemy.enemy_state = ENEMY_STATE::ENEMY_MOVING;
	enemy.enemy_type = ENEMY_TYPE::GROUND;
	enemy.speed = speed;
	enemy.transform_handle = create_transform(pos, go_globals.z_positions[ENEMY_Z_POS_KEY], glm::vec2(1), 0, 0, -1);
	enemy.quad_render_handle = create_quad_render(enemy.transform_handle, create_color(165, 42, 42), enemy_t::GROUND_WIDTH, enemy_t::GROUND_HEIGHT, false, 0, -1);
	enemy.rb_handle = create_rigidbody(enemy.transform_handle, false, enemy_t::GROUND_WIDTH, enemy_t::GROUND_HEIGHT, true, PHYS_ENEMY, true, true);
	enemy.width = enemy_t::GROUND_WIDTH;
	enemy.height = enemy_t::GROUND_HEIGHT; 
	go_globals.enemies.push_back(enemy);
}

void create_air_enemy(glm::vec2 pos, float speed) {
	enemy_t enemy;
	enemy.handle = enemy_cnt++;
	enemy.dir = MOVE_DIR::NONE;
	enemy.enemy_state = ENEMY_STATE::ENEMY_MOVING;
	enemy.enemy_type = ENEMY_TYPE::AIR;
	enemy.speed = speed;
	enemy.transform_handle = create_transform(pos, go_globals.z_positions[ENEMY_Z_POS_KEY], glm::vec2(1), 0, 0, -1);
	enemy.quad_render_handle = create_quad_render(enemy.transform_handle, create_color(235,234,235), enemy_t::AIR_WIDTH, enemy_t::AIR_HEIGHT, false, 0, -1);
	enemy.rb_handle = create_rigidbody(enemy.transform_handle, false, enemy_t::AIR_WIDTH, enemy_t::AIR_HEIGHT, true, PHYS_ENEMY, true, true);
	enemy.width = enemy_t::AIR_WIDTH;
	enemy.height = enemy_t::AIR_HEIGHT;
	go_globals.enemies.push_back(enemy);
}

static void mark_enemy_for_deletion(int handle) {
	go_globals.enemies_to_delete.push_back(handle);
	gun_t::enemy_died_handles.push_back(handle);
}

const float enemy_t::DIST_TO_BASE = 150.f;
const float enemy_t::TIME_BETWEEN_SHOTS = 2.f;
void update_enemy(enemy_t& enemy) {
	transform_t* transform = get_transform(enemy.transform_handle);
	game_assert_msg(transform, "transform for enemy not found");

	if (enemy.enemy_state == ENEMY_STATE::ENEMY_MOVING) {
		if (enemy.enemy_type == ENEMY_TYPE::GROUND) {
			transform->global_position += glm::vec2(enemy.speed * ((int)enemy.dir) * game::time_t::delta_time, 0);

			float closest_distance = FLT_MAX;
			for (int i = 0; i < go_globals.gun_bases.size(); i++) {
				int base_transform_handle = go_globals.gun_bases[i].transform_handle;
				transform_t* base_transform = get_transform(base_transform_handle);
				game_assert_msg(base_transform, "base transform not found");
				float base_dist = abs(transform->global_position.x - base_transform->global_position.x);
				if (base_dist < enemy_t::DIST_TO_BASE && base_dist < closest_distance) {
					closest_distance = base_dist;
					enemy.closest_base.handle = go_globals.gun_bases[i].handle;
					enemy.closest_base.transform_handle = go_globals.gun_bases[i].transform_handle;
					enemy.enemy_state = ENEMY_STATE::ENEMY_SHOOTING;
				}
			}

		} else if (enemy.enemy_type == ENEMY_TYPE::AIR) {
			air_enemy_specific_info_t& air = enemy.air;

			if (std::find(enemy_t::deleted_base_handles.begin(), enemy_t::deleted_base_handles.end(), enemy.closest_base.handle) != enemy_t::deleted_base_handles.end()) {
				enemy.closest_base.handle = -1;
			}
			if (enemy.closest_base.handle == -1) {
				float closest_distance = FLT_MAX;
				for (int i = 0; i < go_globals.gun_bases.size(); i++) {
					int base_transform_handle = go_globals.gun_bases[i].transform_handle;
					transform_t* base_transform = get_transform(base_transform_handle);
					game_assert_msg(base_transform, "base transform not found");
					float base_dist = abs(transform->global_position.x - base_transform->global_position.x);
					if (base_dist < closest_distance) {
						closest_distance = base_dist;
						enemy.closest_base.handle = go_globals.gun_bases[i].handle;
						enemy.closest_base.transform_handle = go_globals.gun_bases[i].transform_handle;
						air.at_top_of_screen = false;
						air.target_loc_at_top_of_screen = glm::vec2(-1, -1);
					}
				}
			}

			if (enemy.closest_base.handle != -1) {
				transform_t* base_transform = get_transform(enemy.closest_base.transform_handle);
				game_assert_msg(base_transform, "base transform not found");
				glm::vec2 diff = base_transform->global_position - transform->global_position;
				glm::vec2 move_dir = glm::normalize(diff);
				transform->global_position += move_dir * enemy.speed * (float)game::time_t::delta_time;
				float base_dist = abs(transform->global_position.x - base_transform->global_position.x);
				if (base_dist < enemy_t::DIST_TO_BASE) {
					enemy.enemy_state = ENEMY_STATE::ENEMY_SHOOTING;
				}
			} else {
				glm::vec2& target = air.target_loc_at_top_of_screen;
				if (target == glm::vec2(-1, -1)) {
					target.x = (globals.window.window_width * 0.05f) + (rand() % static_cast<int>(globals.window.window_width * 0.9f));
					target.y = globals.window.window_height - enemy.height - 20.f - (rand() % 45);
				}
				
				if (glm::distance(target, transform->global_position) <= 2.f && !air.at_top_of_screen) {
					air.at_top_of_screen = true;
					air.dir = (rand() % 2) == 0 ? MOVE_DIR::RIGHT : MOVE_DIR::LEFT;
				}

				if (air.at_top_of_screen) {
					transform->global_position += glm::vec2(enemy.speed * ((int)air.dir) * game::time_t::delta_time, 0);
					float top_of_screen_x = air.target_loc_at_top_of_screen.x;
					float mindless_move_x_extent = globals.window.window_width * .4f;
					if (((transform->global_position.x <= top_of_screen_x - mindless_move_x_extent) || (transform->global_position.x >= top_of_screen_x + mindless_move_x_extent)) && (game::time_t::game_cur_time - air.last_time_changed_dir > 1.f)) {
						air.dir = static_cast<MOVE_DIR>(-1 * (int)air.dir);
						air.last_time_changed_dir = game::time_t::game_cur_time;
					}
				} else {
					glm::vec2 move_dir = glm::normalize(target - transform->global_position);
					transform->global_position += move_dir * enemy.speed * (float)game::time_t::delta_time;		
				}
			}
		}	
	} else if (enemy.enemy_state == ENEMY_STATE::ENEMY_SHOOTING) {
		if (std::find(enemy_t::deleted_base_handles.begin(), enemy_t::deleted_base_handles.end(), enemy.closest_base.handle) != enemy_t::deleted_base_handles.end()) {
			enemy.enemy_state = ENEMY_STATE::ENEMY_MOVING;
			enemy.closest_base.handle = -1;
			enemy.closest_base.transform_handle = -1;
		} else {
			transform_t* base_transform = get_transform(enemy.closest_base.transform_handle);
			game_assert_msg(base_transform, "base transform not found");

			if (enemy.last_shoot_time + enemy_t::TIME_BETWEEN_SHOTS < game::time_t::game_cur_time) {
				enemy.last_shoot_time = game::time_t::game_cur_time;
				glm::vec2 dir = glm::normalize(base_transform->global_position - transform->global_position);
				glm::vec2 pos = transform->global_position;
				create_enemy_bullet(pos, dir, 800.f);
			}
		}
	}

	std::vector<kin_w_kin_col_t> cols = get_from_kin_w_kin_cols(enemy.rb_handle, PHYS_ENEMY);
	for (kin_w_kin_col_t& col : cols) {
		if (col.kin_type1 == PHYS_BULLET || col.kin_type2 == PHYS_BULLET) {
			enemy.health -= 30;
			if (enemy.health <= 0) {
				add_store_credit(10);
				mark_enemy_for_deletion(enemy.handle);
				go_globals.score.enemies_left_to_kill = MAX(0, go_globals.score.enemies_left_to_kill-1);
				return;
			}
		}
	}

	float health_left_pct = enemy.health / 100.f;
	glm::vec2 health_bar_left = world_pos_to_screen(glm::vec2(transform->global_position.x - (enemy.width / 2), transform->global_position.y + (enemy.height/2) + 20));
	glm::vec2 health_bar_dim = world_vec_to_screen_vec(glm::vec2(enemy.width, 10));

	style_t grey_bar;
	grey_bar.background_color = create_color(128, 128, 128, 1);
	push_style(grey_bar);
	char container_bck[256]{};
	sprintf(container_bck, "enemy_%i_bck", enemy.handle);
	create_absolute_container(health_bar_left.x, health_bar_left.y, health_bar_dim.x, health_bar_dim.y, WIDGET_SIZE::PIXEL_BASED, WIDGET_SIZE::PIXEL_BASED, container_bck);
	pop_style();
	
	char container[256]{};
	sprintf(container_bck, "enemy_%i", enemy.handle);
	style_t health_bar;
	health_bar.background_color = health_left_pct * create_color(0,255,0,1) + ((1 - health_left_pct) * create_color(255,0,0,1));
	push_style(health_bar);
	create_absolute_container(health_bar_left.x, health_bar_left.y, health_bar_dim.x * health_left_pct, health_bar_dim.y, WIDGET_SIZE::PIXEL_BASED, WIDGET_SIZE::PIXEL_BASED, container);
	pop_style();
}

void delete_enemy_by_index(int idx, const int handle) {
	enemy_t& enemy = go_globals.enemies[idx];
	if (enemy.handle == handle) {
		delete_transform(enemy.transform_handle);
		delete_rigidbody(enemy.rb_handle);
		delete_quad_render(enemy.quad_render_handle);
		go_globals.enemies.erase(go_globals.enemies.begin() + idx);
	}
}

void delete_enemy(int enemy_handle) {
	for (int i = 0; i < go_globals.enemies.size(); i++) {
		if (go_globals.enemies[i].handle == enemy_handle) {
			delete_transform(go_globals.enemies[i].transform_handle);
			delete_rigidbody(go_globals.enemies[i].rb_handle);
			delete_quad_render(go_globals.enemies[i].quad_render_handle);
			go_globals.enemies.erase(go_globals.enemies.begin() + i);
			gun_t::enemy_died_handles.push_back(enemy_handle);
		}
	}
}