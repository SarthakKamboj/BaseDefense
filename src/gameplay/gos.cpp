#include "gos.h"

#include "gfx/quad.h"
#include "utils/general.h"
#include "physics/physics.h"
#include "constants.h"
#include "utils/time.h"
#include "globals.h"
#include "store.h"
#include "ui/ui.h"
#include "preview_manager.h"

#include <vector>
#include <algorithm>
#include <limits.h>

extern bool paused;

static glm::vec3 invalid_placement_color = create_color(144,66,71);
static glm::vec3 valid_placement_color = create_color(45,45,45);

static const float PREVIEW_MOVE_SPEED = 300.f;

extern inventory_t inventory;
extern store_t store;
extern preview_state_t preview_state;

score_t score;

extern float panel_left;

extern globals_t globals;

static std::vector<base_t> gun_bases;
static std::vector<base_extension_t> attached_base_exts;
static std::vector<gun_t> attached_guns;
static std::vector<bullet_t> bullets;
static std::vector<enemy_bullet_t> enemy_bullets;
static std::vector<attachment_t> attachments;

static std::vector<int> bullets_to_delete;
static std::vector<int> enemy_bullets_to_delete;
static std::vector<int> bases_to_delete;
static std::vector<int> enemies_to_delete;

static std::vector<enemy_spawner_t> enemy_spawners;
static std::vector<enemy_t> enemies;

static gun_t preview_gun;
static base_extension_t preview_base_ext;
static base_t preview_base;

void init_preview_items() {
	init_preview_mode();
	init_preview_gun();
	init_base_ext_preview();
	init_preview_base();
}

const int base_t::WIDTH = 100;
const int base_t::HEIGHT = 300;

void init_preview_base() {
	preview_base.handle = -1;

	preview_base.transform_handle = create_transform(glm::vec3(0), glm::vec3(1), 0.f, 0.f);
	preview_base.quad_render_handle = create_quad_render(preview_base.transform_handle, create_color(60,90,30), base_t::WIDTH, base_t::HEIGHT, false, 0.f, -1);
	preview_base.rb_handle = create_rigidbody(preview_base.transform_handle, false, base_t::WIDTH, base_t::HEIGHT, true, PHYS_NONE, true, false);
}

void update_preview_base() {	
	quad_render_t* quad_render = get_quad_render(preview_base.quad_render_handle);
	game_assert_msg(quad_render, "quad render for base not found");	

	if (preview_state.cur_mode != PREVIEW_MODE::PREVIEW_BASE || store.open || paused) {
		quad_render->render = false;
		return;
	}

	transform_t* preview_transform = get_transform(preview_base.transform_handle);
	game_assert_msg(preview_transform, "could not find transform for preview base");

	if (!globals.window.user_input.game_controller) {
		glm::vec2 mouse = mouse_to_world_pos();
		preview_transform->global_position = glm::vec3(mouse.x, base_t::HEIGHT * 0.5f, 0.f);
	} else {
		float cur_x = preview_transform->global_position.x;
		float delta_x = globals.window.user_input.controller_x_axis * PREVIEW_MOVE_SPEED * game::time_t::delta_time;
		preview_transform->global_position = glm::vec3(cur_x + delta_x, base_t::HEIGHT * 0.5f, 0.f);
	}

	if (inventory.num_bases == 0) {
		quad_render->color = invalid_placement_color;
	} else {
		quad_render->color = valid_placement_color;
	}

	// if (globals.window.user_input.left_mouse_release && inventory.num_bases > 0) {
	bool released_preview_button = get_released(LEFT_MOUSE) || get_released(CONTROLLER_Y);
	if (released_preview_button && inventory.num_bases > 0) {
		create_base(preview_transform->global_position);
		inventory.num_bases--;
	}

	// bool left_down = globals.window.user_input.left_mouse_down;
	bool preview_mode_on = get_down(LEFT_MOUSE) || get_down(CONTROLLER_Y);
	quad_render->render = preview_mode_on;

	update_hierarchy_based_on_globals();
}

void create_base(glm::vec3 pos) {
	static int cnt = 0;
	base_t gun_base;
	gun_base.handle = cnt++;

	gun_base.transform_handle = create_transform(pos, glm::vec3(1), 0.f, 0.f);
	gun_base.quad_render_handle = create_quad_render(gun_base.transform_handle, create_color(59,74,94), base_t::WIDTH, base_t::HEIGHT, false, 0.f, -1);
	gun_base.rb_handle = create_rigidbody(gun_base.transform_handle, false, base_t::WIDTH, base_t::HEIGHT, true, PHYS_BASE, true, true);

	gun_base.previewing = false;

	gun_base.attachment_handles[0] = create_attachment(glm::vec3(-base_t::WIDTH * 0.4f, 0, 0), true, ATTMNT_GUN | ATTMNT_BASE_EXT, &gun_base, NULL);
	gun_base.attachment_handles[1] = create_attachment(glm::vec3(base_t::WIDTH * 0.4f, 0, 0), false, ATTMNT_GUN | ATTMNT_BASE_EXT, &gun_base, NULL);
	gun_base.attachment_handles[2] = create_attachment(glm::vec3(0, base_t::HEIGHT * 0.4f, 0), false, ATTMNT_BASE_EXT, &gun_base, NULL);

	gun_bases.push_back(gun_base);
}

static void mark_base_for_deletion(int handle) {
	bases_to_delete.push_back(handle);
	enemy_t::deleted_base_handles.push_back(handle);
}

void update_base(base_t& base) {

	transform_t* transform = get_transform(base.transform_handle);
	game_assert_msg(transform, "transform for base not found");	

	std::vector<kin_w_kin_col_t> cols = get_from_kin_w_kin_cols(base.rb_handle, PHYS_BASE);
	for (kin_w_kin_col_t& col : cols) {
		if (col.kin_type1 == PHYS_ENEMY_BULLET || col.kin_type2 == PHYS_ENEMY_BULLET) {
			base.base_health -= 20;
	
			if (base.base_health <= 0) {
				mark_base_for_deletion(base.handle);
			}
			return;
		}
	}

	glm::vec2 bar_left(transform->global_position.x - (base_t::WIDTH / 2), transform->global_position.y + (base_t::HEIGHT / 2) + 40);

	glm::vec2 screen_bar_left_pos = world_pos_to_screen(bar_left);
	glm::vec2 screen_bar_dim = world_vec_to_screen_vec(glm::vec2(base_t::WIDTH, 20));
	
	style_t grey_bar;
	grey_bar.background_color = create_color(128, 128, 128, 1);
	push_style(grey_bar);
	char container_bck_name[256]{};
	sprintf(container_bck_name, "base_%i_bkd", base.handle);
	create_absolute_container(screen_bar_left_pos.x, screen_bar_left_pos.y, screen_bar_dim.x, screen_bar_dim.y, WIDGET_SIZE::PIXEL_BASED, WIDGET_SIZE::PIXEL_BASED, container_bck_name);
	pop_style();

	float weight = base.base_health / 100.f;
	style_t health_bar;
	health_bar.background_color = weight * create_color(0,255,0,1) + ((1 - weight) * create_color(255,0,0,1));
	push_style(health_bar);

	char container_name[256]{};
	sprintf(container_name, "base_%i", base.handle);
	create_absolute_container(screen_bar_left_pos.x, screen_bar_left_pos.y, screen_bar_dim.x * weight, screen_bar_dim.y, WIDGET_SIZE::PIXEL_BASED, WIDGET_SIZE::PIXEL_BASED, container_name);

	pop_style();
}

static void delete_base_by_index(int idx, const int handle) {
	base_t& base = gun_bases[idx];
	if (base.handle == gun_bases[idx].handle) {
		delete_transform(base.transform_handle);
		delete_rigidbody(base.rb_handle);
		delete_quad_render(base.quad_render_handle);
	
		delete_attachment(base.attachment_handles[0]);
		delete_attachment(base.attachment_handles[1]);
		delete_attachment(base.attachment_handles[2]);

		gun_bases.erase(gun_bases.begin() + idx);
	}
}

void delete_base(base_t& base) {
	for (int i = 0; i < gun_bases.size(); i++) {
		if (base.handle == gun_bases[i].handle) {
			delete_transform(base.transform_handle);
			delete_rigidbody(base.rb_handle);
			delete_quad_render(base.quad_render_handle);
		
			delete_attachment(base.attachment_handles[0]);
			delete_attachment(base.attachment_handles[1]);
			delete_attachment(base.attachment_handles[2]);

			gun_bases.erase(gun_bases.begin() + i);
			enemy_t::deleted_base_handles.push_back(base.handle);
		}
	}
}

const int attachment_t::WIDTH = 10;
const int attachment_t::HEIGHT = 10;

int create_attachment(glm::vec3 pos, bool facing_left, ATTACHMENT_TYPE attmt_types, base_t* base, base_extension_t* ext) {

	game_assert_msg(base || ext, "neither base or base extension specified");
	game_assert_msg((base && !ext) || (!base && ext), "cannot specify both base and base ext for 1 attachment");

	static int cnt = 0;
	attachment_t attmt;
	attmt.handle = cnt++;
	attmt.attachment_types = attmt_types;
	attmt.facing_left = facing_left;
	if (base) {
		attmt.transform_handle = create_transform(pos, glm::vec3(1), 0, 0, base->transform_handle);
	} else {
		attmt.transform_handle = create_transform(pos, glm::vec3(1), 0, 0, ext->transform_handle);
	}
	attmt.quad_render_handle = create_quad_render(attmt.transform_handle, glm::vec3(1,0,0), attachment_t::WIDTH, attachment_t::HEIGHT, false, 0, -1);
	attachments.push_back(attmt);
	return attmt.handle;
}

void update_attachment(attachment_t& attachment) {
	if (attachment.attached) return;
	transform_t* attachment_transform = get_transform(attachment.transform_handle);
	game_assert_msg(attachment_transform, "could not find transform of attachment pt");

	if (preview_state.cur_mode != PREVIEW_GUN && preview_state.cur_mode != PREVIEW_BASE_EXT) return;
	if (preview_state.cur_mode == PREVIEW_GUN && (attachment.attachment_types & ATTMNT_GUN) == 0) return;
	if (preview_state.cur_mode == PREVIEW_BASE_EXT && (attachment.attachment_types & ATTMNT_BASE_EXT) == 0) return;

	transform_t* preview_transform = preview_state.cur_mode == PREVIEW_GUN ? get_transform(preview_gun.transform_handle) : get_transform(preview_base_ext.transform_handle);
	game_assert_msg(preview_transform, "preview transform not found");

	glm::vec3 preview_obj_pos = glm::vec3(preview_transform->global_position.x, preview_transform->global_position.y, 0);
	if (glm::distance(preview_obj_pos, attachment_transform->global_position) > 20) return;

	if (preview_state.cur_mode == PREVIEW_MODE::PREVIEW_GUN) {
		preview_gun.free = false;
		preview_gun.attachment_handle = attachment.handle;
		preview_transform->global_position = attachment_transform->global_position;
	} else if (preview_state.cur_mode == PREVIEW_MODE::PREVIEW_BASE_EXT) {
		preview_base_ext.attachment_handle = attachment.handle;
		preview_base_ext.free = false;
		preview_transform->global_position = attachment_transform->global_position;
	}

	update_hierarchy_based_on_globals();
}

void delete_attachment(int handle) {
	if (handle == -1) return;
	for (int i = 0; i < attachments.size(); i++) {
		if (attachments[i].handle == handle) {
			delete_attachment(attachments[i]);
		}
	}
}

void delete_attachment(attachment_t& att) {
	for (int i = 0; i < attachments.size(); i++) {
		if (att.handle == attachments[i].handle) {
			delete_transform(att.transform_handle);
			delete_quad_render(att.quad_render_handle);

			if (att.attached) {
				for (int i = 0; i < attached_guns.size(); i++) {
					if (attached_guns[i].attachment_handle == att.handle) {
						delete_attached_gun(attached_guns[i]);
					}
				}

				for (int i = 0; i < attached_base_exts.size(); i++) {
					if (attached_base_exts[i].attachment_handle == att.handle) {
						delete_base_ext(attached_base_exts[i]);
					}
				}
			}

			attachments.erase(attachments.begin() + i);
		}
	}
}

attachment_t* get_attachment(int handle) {
	for (attachment_t& att : attachments) {
		if (att.handle == handle) {
			return &att;
		}
	}
	return NULL;
}

const int base_extension_t::WIDTH = 100;
const int base_extension_t::HEIGHT = 40;
int create_base_ext(glm::vec3 pos) {
	static int cnt = 0;
	base_extension_t base_ext;
	base_ext.handle = cnt++;

	base_ext.transform_handle = create_transform(pos, glm::vec3(1), 0.f, 0.f);
	base_ext.quad_render_handle = create_quad_render(base_ext.transform_handle, create_color(240,74,94), base_extension_t::WIDTH, base_extension_t::HEIGHT, false, 0.f, -1);
	base_ext.rb_handle = create_rigidbody(base_ext.transform_handle, false, base_extension_t::WIDTH, base_extension_t::HEIGHT, true, PHYS_NONE, false, true);
	base_ext.previewing = false;
	base_ext.free = false;
	base_ext.attachment_handle = preview_base_ext.attachment_handle;

	attachment_t* att = get_attachment(base_ext.attachment_handle);
	game_assert_msg(att, "attachment not found to make base ext");
	att->attached = true;


	glm::vec3 offset(base_extension_t::WIDTH * 0.4f, 0, 0);
	base_ext.attachment_handles[0] = create_attachment(offset, false, ATTMNT_GUN, NULL, &base_ext);
	base_ext.attachment_handles[1] = create_attachment(-offset, true, ATTMNT_GUN, NULL, &base_ext);

	attached_base_exts.push_back(base_ext);
	return base_ext.handle;
}

void update_base_ext(base_extension_t& attachment) {

}

void delete_base_ext(base_extension_t& attachment) {
	for (int i = 0; i < attached_base_exts.size(); i++) {
		if (attachment.handle == attached_base_exts[i].handle) {
			delete_transform(attachment.transform_handle);
			delete_rigidbody(attachment.rb_handle);
			delete_quad_render(attachment.quad_render_handle);

			delete_attachment(attachment.attachment_handles[0]);
			delete_attachment(attachment.attachment_handles[1]);

			attached_base_exts.erase(attached_base_exts.begin() + i);
		}
	}	
}

void init_base_ext_preview() {
	preview_base_ext.handle = -1;

	preview_base_ext.transform_handle = create_transform(glm::vec3(0), glm::vec3(1), 0.f, 0.f);
	preview_base_ext.quad_render_handle = create_quad_render(preview_base_ext.transform_handle, create_color(145, 145, 145), base_extension_t::WIDTH, base_extension_t::HEIGHT, false, 0.f, -1);
	preview_base_ext.rb_handle = create_rigidbody(preview_base_ext.transform_handle, false, base_extension_t::WIDTH, base_extension_t::HEIGHT, true, PHYS_NONE, true, false);
}

void update_preview_base_ext() {
	// bool left_down = globals.window.user_input.left_mouse_down;
	// bool left_release = globals.window.user_input.left_mouse_release;

	bool preview_down = get_down(LEFT_MOUSE) || get_down(CONTROLLER_Y);
	bool preview_btn_released = get_released(LEFT_MOUSE) || get_released(CONTROLLER_Y);

	quad_render_t* preview_quad = get_quad_render(preview_base_ext.quad_render_handle);
	game_assert_msg(preview_quad, "quad render for preview attachment not found");

	if (preview_state.cur_mode != PREVIEW_MODE::PREVIEW_BASE_EXT || store.open || paused) {
		preview_quad->render = false;
		return;
	}

	preview_quad->render = preview_down;

	if (inventory.num_base_exts == 0) {
		preview_quad->color = invalid_placement_color;
	} else {
		preview_quad->color = valid_placement_color;
	}

	transform_t* transform = get_transform(preview_base_ext.transform_handle);
	game_assert_msg(transform, "transform of preview attachment not found");
	if(preview_btn_released && !preview_base_ext.free && inventory.num_base_exts > 0) {
		preview_base_ext.free = true;
		create_base_ext(transform->global_position);
		inventory.num_base_exts--;
		return;
	}	

	if (!preview_down) {	
		return;
	}

	if (preview_base_ext.free) {
		if (is_controller_connected()) {
			// float cur_x = preview_transform->global_position.x;
			// float cur_y = preview_transform->global_position.x;
			float delta_x = globals.window.user_input.controller_x_axis * PREVIEW_MOVE_SPEED * game::time_t::delta_time;
			float delta_y = globals.window.user_input.controller_y_axis * PREVIEW_MOVE_SPEED * game::time_t::delta_time;

			transform->global_position += glm::vec3(delta_x, delta_y, 0);
		} else {
			glm::vec2 mouse = mouse_to_world_pos();
			transform->global_position = glm::vec3(mouse.x, mouse.y, 0);
		}
	}	
}

base_extension_t* get_base_ext(int handle) {
	for (int i = 0; i < attached_base_exts.size(); i++) {
		if (handle == attached_base_exts[i].handle) {
			return &attached_base_exts[i];
		}
	}
	return NULL;
}

void init_preview_gun() {
	preview_gun.handle = -1;

	preview_gun.attachment_handle = -1;

	preview_gun.transform_handle = create_transform(glm::vec3(0), glm::vec3(1), 0.f, 0.f, -1);
	preview_gun.quad_render_handle = create_quad_render(preview_gun.transform_handle, valid_placement_color, gun_t::WIDTH, gun_t::HEIGHT, false, 0.f, -1);
	preview_gun.rb_handle = create_rigidbody(preview_gun.transform_handle, false, gun_t::WIDTH, gun_t::HEIGHT, true, PHYS_NONE, false, false);
}

const int gun_t::WIDTH = 80;
const int gun_t::HEIGHT = 20;
void create_attached_gun(int attachment_handle, bool facing_left, float fire_rate) {
	static int cnt = 0;

	gun_t gun;
	gun.handle = cnt++;
	gun.attachment_handle = preview_gun.attachment_handle;

	attachment_t* att = get_attachment(gun.attachment_handle);
	game_assert_msg(att, "attachment not found to make gun");
	att->attached = true;

	transform_t* att_transform = get_transform(att->transform_handle);
	game_assert_msg(att_transform, "transform of attachment not found");
	// att_transform->ro

	gun.fire_rate = fire_rate;
	gun.facing_left = facing_left;

	int multiplier = facing_left ? -1 : 1;
	glm::vec3 gun_pos = glm::vec3(multiplier * gun_t::WIDTH / 2.f, 0, 0);

	gun.transform_handle = create_transform(gun_pos, glm::vec3(1), 0.f, 0.f, att->transform_handle);
	transform_t* g = get_transform(gun.transform_handle);
	gun.quad_render_handle = create_quad_render(gun.transform_handle, create_color(30,0,120), gun_t::WIDTH, gun_t::HEIGHT, false, 0.f, -1);
	gun.rb_handle = create_rigidbody(gun.transform_handle, false, gun_t::WIDTH, gun_t::HEIGHT, true, PHYS_NONE, true, true);

	gun.previewing = false;
	gun.free = false;

	if (gun.facing_left) {
		gun.prev_enemy_last_target_dir = glm::vec2(-1,0);
	} else {
		gun.prev_enemy_last_target_dir = glm::vec2(1,0);
	}
	gun.last_target_dir = gun.prev_enemy_last_target_dir;

	attached_guns.push_back(gun);
}

void update_preview_gun() {
	// bool left_down = globals.window.user_input.left_mouse_down;
	// bool left_release = globals.window.user_input.left_mouse_release;
	bool preview_down = get_down(LEFT_MOUSE) || get_down(CONTROLLER_Y);
	bool preview_btn_released = get_released(LEFT_MOUSE) || get_released(CONTROLLER_Y);

	quad_render_t* preview_quad = get_quad_render(preview_gun.quad_render_handle);
	game_assert_msg(preview_quad, "quad render for preview gun not found");

	if (preview_state.cur_mode != PREVIEW_MODE::PREVIEW_GUN || store.open || paused) {
		preview_quad->render = false;
		return;
	}

	preview_quad->render = preview_down;

	if (inventory.num_guns == 0) {
		preview_quad->color = invalid_placement_color;
	} else {
		preview_quad->color = valid_placement_color;
	}

	if(preview_btn_released && !preview_gun.free && inventory.num_guns > 0) {
		attachment_t* att = get_attachment(preview_gun.attachment_handle);
		game_assert_msg(att, "attachment to create gun not found");
		create_attached_gun(preview_gun.attachment_handle, att->facing_left, 2.f);
		inventory.num_guns--;
		preview_gun.free = true;
		return;
	}	

	if (!preview_down) {	
		return;
	}

	if (preview_gun.free) {
		transform_t* transform = get_transform(preview_gun.transform_handle);
		game_assert_msg(transform, "transform of preview gun not found");

		if (is_controller_connected()) {
			float delta_x = globals.window.user_input.controller_x_axis * PREVIEW_MOVE_SPEED * game::time_t::delta_time;
			float delta_y = globals.window.user_input.controller_y_axis * PREVIEW_MOVE_SPEED * game::time_t::delta_time;

			transform->global_position += glm::vec3(delta_x, delta_y, 0);
		} else {
			glm::vec2 mouse = mouse_to_world_pos();
			transform->global_position = glm::vec3(mouse.x, mouse.y, 0);
		}
	}
}

std::vector<int> gun_t::enemy_died_handles;
const float gun_t::RETARGET_ANIM_TIME = 1;
void update_attached_gun(gun_t& gun) {	
	set_quad_color(gun.quad_render_handle, create_color(60,0,240));

	if (gun.closest_enemy.handle != -1 && std::find(gun_t::enemy_died_handles.begin(), gun_t::enemy_died_handles.end(), gun.closest_enemy.handle) != gun_t::enemy_died_handles.end()) {
		gun.closest_enemy.handle = -1;
		gun.closest_enemy.transform_handle = -1;
	}	

	transform_t* gun_transform = get_transform(gun.transform_handle);
	game_assert_msg(gun_transform, "transform for gun not found");

	attachment_t* att = get_attachment(gun.attachment_handle);
	game_assert_msg(att, "att for attached gun not found");
	transform_t* attachment_transform = get_transform(att->transform_handle);
	game_assert_msg(attachment_transform, "transform for gun's attach pt not found");
	
	if (gun.closest_enemy.handle == -1) {
		float closest = FLT_MAX;
		for (int i = 0; i < enemies.size(); i++) {
			transform_t* enemy_transform = get_transform(enemies[i].transform_handle);
			game_assert_msg(enemy_transform, "enemy transform not found");
			float distance = glm::distance(enemy_transform->global_position, attachment_transform->global_position);
			if (distance < closest && std::find(gun_t::enemy_died_handles.begin(), gun_t::enemy_died_handles.end(), enemies[i].handle) == gun_t::enemy_died_handles.end()) {
				closest = distance;
				gun.closest_enemy.handle = enemies[i].handle;
				gun.closest_enemy.transform_handle = enemies[i].transform_handle;
			}
		}

		gun.last_retarget_time = game::time_t::game_cur_time;
		gun.prev_enemy_last_target_dir = gun.last_target_dir;
	}
	
	glm::vec2 gun_facing_dir;
	time_count_t t = fmin(1, (game::time_t::game_cur_time - gun.last_retarget_time) / gun_t::RETARGET_ANIM_TIME);
	if (gun.closest_enemy.handle == -1) {
		gun_facing_dir = gun.facing_left ? glm::vec2(-1,0) : glm::vec2(1,0);
	} else {
		transform_t* enemy_transform = get_transform(gun.closest_enemy.transform_handle);
		game_assert_msg(enemy_transform, "enemy transform not found");

		glm::vec2 to_enemy = glm::normalize(glm::vec2(enemy_transform->global_position.x - attachment_transform->global_position.x, enemy_transform->global_position.y - attachment_transform->global_position.y));
		gun_facing_dir = lerp(gun.prev_enemy_last_target_dir, to_enemy, t);
	}

	glm::vec2 horizontal = glm::vec2(-1, 0);
	glm::vec2 vertical = glm::vec2(0, -1);
	if (!gun.facing_left) {
		horizontal = glm::vec2(1, 0);
		vertical = glm::vec2(0, 1);
	}
	float cos_z = glm::dot(gun_facing_dir, horizontal);
	float z_rad = acos(cos_z);
	glm::vec3 rot = attachment_transform->local_rotation;
	rot.z = glm::degrees(z_rad);

	float below_gun = glm::dot(gun_facing_dir, vertical);
	if (below_gun < 0) {
		rot.z *= -1;
	}
	set_local_rot(attachment_transform, rot);

	float time_between_fires = 1 / gun.fire_rate;
	if (gun.closest_enemy.handle != -1 && gun.time_since_last_fire + time_between_fires < game::time_t::game_cur_time && t >= 1) {
		gun.time_since_last_fire = game::time_t::game_cur_time;
		create_bullet(attachment_transform->global_position, glm::vec3(gun_facing_dir.x, gun_facing_dir.y, 0), 800.f);
	}

	gun.last_target_dir = gun_facing_dir;
}

void delete_attached_gun(gun_t& gun) {
	for (int i = 0; i < attached_guns.size(); i++) {
		if (gun.handle == attached_guns[i].handle) {
			delete_transform(gun.transform_handle);
			delete_rigidbody(gun.rb_handle);
			delete_quad_render(gun.quad_render_handle);
			attached_guns.erase(attached_guns.begin() + i);
		}
	}
}

const float bullet_t::ALIVE_TIME = 1.f;
const int bullet_t::WIDTH = 8;
const int bullet_t::HEIGHT = 8;

void create_bullet(glm::vec3& start_pos, glm::vec3& move_dir, float speed) {
	static int cnt = 0;
	bullet_t bullet;
	bullet.handle = cnt++;

	bullet.transform_handle = create_transform(start_pos, glm::vec3(1), 0, 0, -1);
	bullet.quad_render_handle = create_quad_render(bullet.transform_handle, glm::vec3(1,1,0), bullet_t::WIDTH, bullet_t::HEIGHT, false, 0, -1);
	bullet.rb_handle = create_rigidbody(bullet.transform_handle, false, bullet_t::WIDTH, bullet_t::HEIGHT, true, PHYS_BULLET, true, true);

	bullet.dir = move_dir;
	bullet.speed = speed;
	bullet.creation_time = game::time_t::cur_time;
	bullets.push_back(bullet);
}

void mark_bullet_for_deletion(int bullet_handle) {
	bullets_to_delete.push_back(bullet_handle);
}

void update_bullet(bullet_t& bullet) {
	transform_t* bullet_transform = get_transform(bullet.transform_handle);
	game_assert_msg(bullet_transform, "bullet transform not found");
	bullet_transform->global_position += bullet.dir * glm::vec3(bullet.speed * game::time_t::delta_time);

	std::vector<kin_w_kin_col_t> cols = get_from_kin_w_kin_cols(bullet.rb_handle, PHYS_BULLET);
	for (kin_w_kin_col_t& col : cols) {
		if (col.kin_type1 == PHYS_ENEMY || col.kin_type2 == PHYS_ENEMY) {
			// delete_bullet(bullet);
			mark_bullet_for_deletion(bullet.handle);
			return;
		}
	}

	if (bullet.creation_time + bullet_t::ALIVE_TIME < game::time_t::cur_time) {
		// delete_bullet(bullet);
		mark_bullet_for_deletion(bullet.handle);
	}
}

static void delete_bullet_by_index(int idx, const int handle) {
	bullet_t& bullet = bullets[idx];
	if (bullet.handle == handle) {
		delete_transform(bullet.transform_handle);
		delete_quad_render(bullet.quad_render_handle);
		delete_rigidbody(bullet.rb_handle);
		bullets.erase(bullets.begin() + idx);
		return;
	}
}

void delete_bullet(bullet_t& bullet) {
	for (int i = 0; i < bullets.size(); i++) {
		if (bullets[i].handle == bullet.handle) {
			delete_transform(bullet.transform_handle);
			delete_quad_render(bullet.quad_render_handle);
			delete_rigidbody(bullet.rb_handle);
			bullets.erase(bullets.begin() + i);
			return;
		}
	}
}

std::vector<int> enemy_t::deleted_base_handles;

const int enemy_t::HEIGHT = 80;
const int enemy_t::WIDTH = 40;
void create_enemy(glm::vec3 pos, int dir, float speed) {
	static int cnt = 0;
	enemy_t enemy;
	enemy.handle = cnt++;
	enemy.dir = dir;
	enemy.enemy_state = ENEMY_WALKING;
	enemy.speed = speed;
	enemy.transform_handle = create_transform(pos, glm::vec3(1), 0, 0, -1);
	enemy.quad_render_handle = create_quad_render(enemy.transform_handle, create_color(75, 25, 25), enemy_t::WIDTH, enemy_t::HEIGHT, false, 0, -1);
	enemy.rb_handle = create_rigidbody(enemy.transform_handle, false, enemy_t::WIDTH, enemy_t::HEIGHT, true, PHYS_ENEMY, true, true);
	enemies.push_back(enemy);
}

static void mark_enemy_for_deletion(int handle) {
	enemies_to_delete.push_back(handle);
	gun_t::enemy_died_handles.push_back(handle);
}

const float enemy_t::DIST_TO_BASE = 150.f;
const float enemy_t::TIME_BETWEEN_SHOTS = 2.f;
void update_enemy(enemy_t& enemy) {
	transform_t* transform = get_transform(enemy.transform_handle);
	game_assert_msg(transform, "transform for enemy not found");

	if (enemy.enemy_state == ENEMY_WALKING) {
		transform->global_position += glm::vec3(enemy.speed * enemy.dir * game::time_t::delta_time,0,0);	
		float closest_distance = FLT_MAX;
		for (int i = 0; i < gun_bases.size(); i++) {
			int base_transform_handle = gun_bases[i].transform_handle;
			transform_t* base_transform = get_transform(base_transform_handle);
			game_assert_msg(base_transform, "base transform not found");
			float base_dist = abs(transform->global_position.x - base_transform->global_position.x);
			if (base_dist < enemy_t::DIST_TO_BASE && base_dist < closest_distance) {
				closest_distance = base_dist;
				enemy.closest_base.handle = gun_bases[i].handle;
				enemy.closest_base.transform_handle = gun_bases[i].transform_handle;
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
				glm::vec3 dir = glm::normalize(base_transform->global_position - transform->global_position);
				create_enemy_bullet(transform->global_position, dir, 800.f);
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
				score.enemies_left_to_kill = MAX(0, score.enemies_left_to_kill-1);
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

static void delete_enemy_by_index(int idx, const int handle) {
	enemy_t& enemy = enemies[idx];
	if (enemy.handle == handle) {
		delete_transform(enemy.transform_handle);
		delete_rigidbody(enemy.rb_handle);
		delete_quad_render(enemy.quad_render_handle);
		enemies.erase(enemies.begin() + idx);
	}
}



void delete_enemy(int enemy_handle) {
	for (int i = 0; i < enemies.size(); i++) {
		if (enemies[i].handle == enemy_handle) {
			delete_transform(enemies[i].transform_handle);
			delete_rigidbody(enemies[i].rb_handle);
			delete_quad_render(enemies[i].quad_render_handle);
			enemies.erase(enemies.begin() + i);
			gun_t::enemy_died_handles.push_back(enemy_handle);
		}
	}
}

const int enemy_bullet_t::WIDTH = 8;
const int enemy_bullet_t::HEIGHT = 8;
const float enemy_bullet_t::ALIVE_TIME = 1.f;
void create_enemy_bullet(glm::vec3 pos, glm::vec3 dir, float speed) {
	static int cnt = 0;
	enemy_bullet_t enemy_bullet;
	enemy_bullet.handle = cnt++;
	enemy_bullet.transform_handle = create_transform(pos, glm::vec3(1), 0, 0, -1);
	enemy_bullet.rb_handle = create_rigidbody(enemy_bullet.transform_handle, false, enemy_bullet_t::WIDTH, enemy_bullet_t::HEIGHT, true, PHYS_ENEMY_BULLET, true, true);
	enemy_bullet.quad_render_handle = create_quad_render(enemy_bullet.transform_handle, glm::vec3(0,1,0), enemy_bullet_t::WIDTH, enemy_bullet_t::HEIGHT, false, 0.f, -1);
	enemy_bullet.creation_time = game::time_t::game_cur_time;
	enemy_bullet.speed = speed;
	enemy_bullet.dir = dir;
	enemy_bullets.push_back(enemy_bullet);
}

static void mark_enemy_bullet_for_deletion(int handle) {
	enemy_bullets_to_delete.push_back(handle);
}

void update_enemy_bullet(enemy_bullet_t& bullet) {
	transform_t* bullet_transform = get_transform(bullet.transform_handle);
	game_assert_msg(bullet_transform, "bullet transform not found");
	bullet_transform->global_position += bullet.dir * glm::vec3(bullet.speed * game::time_t::delta_time);

	std::vector<kin_w_kin_col_t> cols = get_from_kin_w_kin_cols(bullet.rb_handle, PHYS_ENEMY_BULLET);
	for (kin_w_kin_col_t& col : cols) {
		if (col.kin_type1 == PHYS_BASE || col.kin_type2 == PHYS_BASE) {
			// delete_enemy_bullet(bullet);
			mark_enemy_bullet_for_deletion(bullet.handle);
			return;
		}
	}

	if (bullet.creation_time + bullet_t::ALIVE_TIME < game::time_t::cur_time) {
		// delete_enemy_bullet(bullet);
		mark_enemy_bullet_for_deletion(bullet.handle);
	}
}

static void delete_enemy_bullet_by_index(int idx, const int enemy_bullet_handle) {
	enemy_bullet_t& bullet = enemy_bullets[idx];
	if (bullet.handle == enemy_bullet_handle) {
		delete_transform(bullet.transform_handle);
		delete_quad_render(bullet.quad_render_handle);
		delete_rigidbody(bullet.rb_handle);
		enemy_bullets.erase(enemy_bullets.begin() + idx);
	}
}

void delete_enemy_bullet(enemy_bullet_t& bullet) {
	for (int i = 0; i < enemy_bullets.size(); i++) {
		if (enemy_bullets[i].handle == bullet.handle) {
			delete_transform(bullet.transform_handle);
			delete_quad_render(bullet.quad_render_handle);
			delete_rigidbody(bullet.rb_handle);
			enemy_bullets.erase(enemy_bullets.begin() + i);
			return;
		}
	}
}

const float enemy_spawner_t::TIME_BETWEEN_SPAWNS = 1.5f;
void create_enemy_spawner(glm::vec3 pos) {
	static int cnt = 0;
	enemy_spawner_t enemy_spawner;	
	enemy_spawner.handle = cnt++;
	enemy_spawner.transform_handle = create_transform(pos, glm::vec3(1), 0, 0, -1);
	enemy_spawner.quad_render_handle = create_quad_render(enemy_spawner.transform_handle, create_color(25,25,75), 60, 140, false, 0, -1);

	enemy_spawners.push_back(enemy_spawner);
}

void update_enemy_spawner(enemy_spawner_t& spawner) {
	spawner.enemy_relative_time += game::time_t::delta_time;
	if (spawner.last_spawn_time + enemy_spawner_t::TIME_BETWEEN_SPAWNS >= spawner.enemy_relative_time) return;
	transform_t* transform = get_transform(spawner.transform_handle);
	game_assert_msg(transform, "transform for enemy spawner not found");
	create_enemy(transform->global_position, 1, 40.f);
	spawner.last_spawn_time = spawner.enemy_relative_time;
}

void update_score() {
	// score.enemies_left_to_kill = MAX(0, 10-num_enemies_killed);
    set_ui_value(std::string("num_enemies_left_to_kill"), std::to_string(score.enemies_left_to_kill));
	if (score.enemies_left_to_kill == 0) {
		globals.scene_manager.queue_level_load = true;
		globals.scene_manager.level_to_load = GAME_OVER_SCREEN_LEVEL;
	}
}

void gos_update() {
	update_preview_base();

	preview_base_ext.free = true;
	preview_gun.free = true;

	for (attachment_t& att : attachments) {
		update_attachment(att);
	}

	update_preview_gun();
	update_preview_base_ext();	

	for (bullet_t& bullet : bullets) {
		update_bullet(bullet);
	}	

	for (enemy_spawner_t& enemy_spawner : enemy_spawners) {
		update_enemy_spawner(enemy_spawner);
	}

	create_absolute_panel("enemies_health_panel", -1);
	for (enemy_t& enemy : enemies) {
		update_enemy(enemy);
	}
	enemy_t::deleted_base_handles.clear();
	end_absolute_panel();

	for (enemy_bullet_t& enemy_bullet : enemy_bullets) {
		update_enemy_bullet(enemy_bullet);
	}

	update_hierarchy_based_on_globals();

	for (gun_t& gun : attached_guns) {
		update_attached_gun(gun);
	}
	gun_t::enemy_died_handles.clear();

	create_absolute_panel("base_health_panel", -1);
	for (base_t& base : gun_bases) {
		update_base(base);
	}
	end_absolute_panel();

	update_hierarchy_based_on_globals();
	update_score();

	gos_update_delete_pass();
}

void gos_update_delete_pass() {
	for (int handle : bullets_to_delete) {
		for (int i = 0; i < bullets.size(); i++) {
			if (bullets[i].handle == handle) {
				delete_bullet_by_index(i, handle);
			}
		}
	}
	bullets_to_delete.clear();

	for (int handle : enemy_bullets_to_delete) {
		for (int i = 0; i < enemy_bullets.size(); i++) {
			if (enemy_bullets[i].handle == handle) {
				delete_enemy_bullet_by_index(i, handle);
			}
		}
	}
	enemy_bullets_to_delete.clear();

	for (int handle : bases_to_delete) {
		for (int i = 0; i < gun_bases.size(); i++) {
			if (gun_bases[i].handle == handle) {
				delete_base_by_index(i, handle);
			}
		}
	}
	bases_to_delete.clear();

	for (int handle : enemies_to_delete) {
		for (int i = 0; i < enemies.size(); i++) {
			if (enemies[i].handle == handle) {
				delete_enemy_by_index(i, handle);
			}
		}
	}
	enemies_to_delete.clear();
}

void delete_gos() {
	attachments.clear();
	attached_guns.clear();
	bullets.clear();
	enemy_spawners.clear();
	enemies.clear();
	gun_bases.clear();
	attached_base_exts.clear();
	enemy_bullets.clear();

	preview_base = base_t();
	preview_base_ext = base_extension_t();
	preview_gun = gun_t();
}