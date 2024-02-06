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

extern go_globals_t go_globals;

std::vector<int> enemy_t::deleted_base_handles;

const int enemy_t::HEIGHT = 120;
const int enemy_t::WIDTH = 40;
void create_enemy(glm::vec2 pos, MOVE_DIR dir, float speed) {
	static int cnt = 0;
	enemy_t enemy;
	enemy.handle = cnt++;
	enemy.dir = dir;
	enemy.enemy_state = ENEMY_WALKING;
	enemy.speed = speed;
	enemy.transform_handle = create_transform(pos, go_globals.z_positions[ENEMY_Z_POS_KEY], glm::vec2(1), 0, 0, -1);
	enemy.quad_render_handle = create_quad_render(enemy.transform_handle, create_color(75, 25, 25), enemy_t::WIDTH, enemy_t::HEIGHT, false, 0, -1);
	enemy.rb_handle = create_rigidbody(enemy.transform_handle, false, enemy_t::WIDTH, enemy_t::HEIGHT, true, PHYS_ENEMY, true, true);
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

	if (enemy.enemy_state == ENEMY_WALKING) {
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
				enemy.enemy_state = ENEMY_SHOOTING;
			}
		}
	} else if (enemy.enemy_state == ENEMY_SHOOTING) {
		if (std::find(enemy_t::deleted_base_handles.begin(), enemy_t::deleted_base_handles.end(), enemy.closest_base.handle) != enemy_t::deleted_base_handles.end()) {
			enemy.enemy_state = ENEMY_WALKING;
		} else {
			transform_t* base_transform = get_transform(enemy.closest_base.transform_handle);
			game_assert_msg(base_transform, "base transform not found");

			if (enemy.last_shoot_time + enemy_t::TIME_BETWEEN_SHOTS < game::time_t::game_cur_time) {
				enemy.last_shoot_time = game::time_t::game_cur_time;
				glm::vec2 dir = glm::normalize(base_transform->global_position - transform->global_position);
				glm::vec2 pos = transform->global_position;
				create_enemy_bullet(glm::vec2(pos.x, pos.y), dir, 800.f);
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
	glm::vec2 health_bar_left = world_pos_to_screen(glm::vec2(transform->global_position.x - enemy_t::WIDTH / 2, transform->global_position.y + enemy_t::HEIGHT/2 + 20));
	glm::vec2 health_bar_dim = world_vec_to_screen_vec(glm::vec2(enemy_t::WIDTH, 10));

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