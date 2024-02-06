#include "gos.h"

#include "gfx/quad.h"
#include "utils/general.h"
#include "physics/physics.h"
#include "constants.h"
#include "utils/time.h"
#include "globals.h"
#include "store.h"
#include "ui/ui_elements.h"
#include "preview_manager.h"
#include "gos_globals.h"
#include "enemies.h"

#include <vector>
#include <algorithm>
#include <limits.h>
#include <map>

extern inventory_t inventory;
extern store_t store;
extern preview_state_t preview_state;

extern globals_t globals;
extern go_globals_t go_globals;

static std::vector<int> bullets_to_delete;
static std::vector<int> enemy_bullets_to_delete;
static std::vector<int> bases_to_delete;

const int base_t::WIDTH = 150;
const int base_t::HEIGHT = 300;

void create_base(glm::vec2 pos) {
	static int cnt = 0;
	base_t gun_base;
	gun_base.handle = cnt++;

	gun_base.transform_handle = create_transform(pos, go_globals.z_positions[BASE_Z_POS_KEY], glm::vec2(1), 0.f, 0.f);
	gun_base.quad_render_handle = create_quad_render(gun_base.transform_handle, create_color(59,74,94), base_t::WIDTH, base_t::HEIGHT, false, 0.f, -1);
	gun_base.rb_handle = create_rigidbody(gun_base.transform_handle, false, base_t::WIDTH, base_t::HEIGHT, true, PHYS_BASE, true, true);

	gun_base.attachment_handles[0] = create_attachment(glm::vec2(-base_t::WIDTH * 0.45f, 0), ATT_PLACEMENT::LEFT, ATTMNT_GUN | ATTMNT_BASE_EXT, &gun_base, NULL);
	gun_base.attachment_handles[1] = create_attachment(glm::vec2(base_t::WIDTH * 0.45f, 0), ATT_PLACEMENT::RIGHT, ATTMNT_GUN | ATTMNT_BASE_EXT, &gun_base, NULL);
	gun_base.attachment_handles[2] = create_attachment(glm::vec2(0, base_t::HEIGHT * 0.45f), ATT_PLACEMENT::TOP, ATTMNT_BASE_EXT, &gun_base, NULL);

	go_globals.gun_bases.push_back(gun_base);
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
	base_t& base = go_globals.gun_bases[idx];
	if (base.handle == go_globals.gun_bases[idx].handle) {
		delete_transform(base.transform_handle);
		delete_rigidbody(base.rb_handle);
		delete_quad_render(base.quad_render_handle);
	
		delete_attachment(base.attachment_handles[0]);
		delete_attachment(base.attachment_handles[1]);
		delete_attachment(base.attachment_handles[2]);

		go_globals.gun_bases.erase(go_globals.gun_bases.begin() + idx);
	}
}

void delete_base(base_t& base) {
	for (int i = 0; i < go_globals.gun_bases.size(); i++) {
		if (base.handle == go_globals.gun_bases[i].handle) {
			delete_transform(base.transform_handle);
			delete_rigidbody(base.rb_handle);
			delete_quad_render(base.quad_render_handle);
		
			delete_attachment(base.attachment_handles[0]);
			delete_attachment(base.attachment_handles[1]);
			delete_attachment(base.attachment_handles[2]);

			go_globals.gun_bases.erase(go_globals.gun_bases.begin() + i);
			enemy_t::deleted_base_handles.push_back(base.handle);
		}
	}
}

const int attachment_t::WIDTH = 10;
const int attachment_t::HEIGHT = 10;

int create_attachment(glm::vec2 pos, ATT_PLACEMENT att_placement, ATTACHMENT_TYPE attmt_types, base_t* base, base_extension_t* ext) {

	game_assert_msg(base || ext, "neither base or base extension specified");
	game_assert_msg((base && !ext) || (!base && ext), "cannot specify both base and base ext for 1 attachment");

	static int cnt = 0;
	attachment_t attmt;
	attmt.handle = cnt++;
	attmt.attachment_types = attmt_types;
	attmt.att_placement = att_placement;
	if (base) {
		attmt.transform_handle = create_transform(pos, go_globals.z_positions[ATT_Z_POS_KEY], glm::vec2(1), 0, 0, base->transform_handle);
	} else {
		attmt.transform_handle = create_transform(pos, go_globals.z_positions[ATT_Z_POS_KEY], glm::vec2(1), 0, 0, ext->transform_handle);
	}
	attmt.quad_render_handle = create_quad_render(attmt.transform_handle, create_color(0,0,255), attachment_t::WIDTH, attachment_t::HEIGHT, false, 0, -1);
	go_globals.attachments.push_back(attmt);
	add_attachment_to_preview_manager(attmt);
	transform_t* t = get_transform(attmt.transform_handle);
	game_assert_msg(t, "transform for att not found");
	return attmt.handle;
}

void update_attachment(attachment_t& attachment) {
	transform_t* attachment_transform = get_transform(attachment.transform_handle);
	game_assert_msg(attachment_transform, "could not find transform of attachment pt");

	if (attachment.attached) return;

	if (preview_state.cur_mode != PREVIEW_GUN && preview_state.cur_mode != PREVIEW_BASE_EXT) return;
	if (preview_state.cur_mode == PREVIEW_GUN && (attachment.attachment_types & ATTMNT_GUN) == 0) return;
	if (preview_state.cur_mode == PREVIEW_BASE_EXT && (attachment.attachment_types & ATTMNT_BASE_EXT) == 0) return;

	transform_t* preview_transform = preview_state.cur_mode == PREVIEW_GUN ? get_transform(preview_state.preview_gun.transform_handle) : get_transform(preview_state.preview_base_ext.transform_handle);
	game_assert_msg(preview_transform, "preview transform not found");

	glm::vec2 preview_obj_pos = glm::vec2(preview_transform->global_position.x, preview_transform->global_position.y);
	glm::vec2 att_pos_vec2 = glm::vec2(attachment_transform->global_position.x, attachment_transform->global_position.y);

	if (glm::distance(preview_obj_pos, att_pos_vec2) > 20) return;

	bool preview_btn_released = get_released(LEFT_MOUSE) || get_released(CONTROLLER_Y);
	if (preview_state.cur_mode == PREVIEW_MODE::PREVIEW_GUN) {
		preview_state.preview_gun.attachment_handle = attachment.handle;
		preview_transform->global_position.x = attachment_transform->global_position.x;
		preview_transform->global_position.y = attachment_transform->global_position.y;
	
		if(preview_btn_released && inventory.num_guns > 0) {
			attachment_t* att = get_attachment(preview_state.preview_gun.attachment_handle);
			game_assert_msg(att, "attachment to create gun not found");
			create_attached_gun(preview_state.preview_gun.attachment_handle, att->att_placement == ATT_PLACEMENT::LEFT, 2.f);
			inventory.num_guns--;
			delete_attachment_from_preview_manager(attachment.handle);
			attachment.attached = true;
			quad_render_t* q = get_quad_render(attachment.quad_render_handle);
			q->render = false;
		}	
	} else if (preview_state.cur_mode == PREVIEW_MODE::PREVIEW_BASE_EXT) {
		preview_state.preview_base_ext.attachment_handle = attachment.handle;
		preview_transform->global_position.x = attachment_transform->global_position.x;
		preview_transform->global_position.y = attachment_transform->global_position.y;

		transform_t* transform = get_transform(preview_state.preview_base_ext.transform_handle);
		game_assert_msg(transform, "transform of preview attachment not found");
		if(preview_btn_released && inventory.num_base_exts > 0) {
			create_base_ext(attachment, preview_state.cur_base_ext_select_mode);
			inventory.num_base_exts--;
			delete_attachment_from_preview_manager(attachment.handle);
			attachment.attached = true;
			quad_render_t* q = get_quad_render(attachment.quad_render_handle);
			game_assert_msg(q, "quad render handle not found");
			q->render = false;
		}
	}

	update_hierarchy_based_on_globals();
}

void delete_attachment(int handle) {
	if (handle == -1) return;
	for (int i = 0; i < go_globals.attachments.size(); i++) {
		if (go_globals.attachments[i].handle == handle) {
			delete_attachment(go_globals.attachments[i]);
			return;
		}
	}
}

void delete_attachment(attachment_t& att) {
	for (int i = 0; i < go_globals.attachments.size(); i++) {
		if (att.handle == go_globals.attachments[i].handle) {
			delete_transform(att.transform_handle);
			delete_quad_render(att.quad_render_handle);

			if (att.attached) {
				for (int i = 0; i < go_globals.attached_guns.size(); i++) {
					if (go_globals.attached_guns[i].attachment_handle == att.handle) {
						delete_attached_gun(go_globals.attached_guns[i]);
					}
				}

				for (int i = 0; i < go_globals.attached_base_exts.size(); i++) {
					if (go_globals.attached_base_exts[i].attachment_handle == att.handle) {
						delete_base_ext(go_globals.attached_base_exts[i]);
					}
				}
			}

			delete_attachment_from_preview_manager(att.handle);
			go_globals.attachments.erase(go_globals.attachments.begin() + i);
			return;
		}
	}
}

attachment_t* get_attachment(int handle) {
	for (attachment_t& att : go_globals.attachments) {
		if (att.handle == handle) {
			return &att;
		}
	}
	return NULL;
}

const int base_extension_t::WIDTH = 100;
const int base_extension_t::HEIGHT = 40;
// int create_base_ext(glm::vec2 pos, BASE_EXT_TYPE type) {
int create_base_ext(attachment_t& att, BASE_EXT_TYPE type) {
	static int cnt = 0;
	base_extension_t base_ext;
	base_ext.handle = cnt++;
	base_ext.base_ext_type = type;

	// glm::vec3 place_pos(pos.x, pos.y, go_globals.z_positions[BASE_EXT_Z_POS_KEY]);
	
	glm::vec2 offset(base_extension_t::WIDTH * 0.45f, base_extension_t::HEIGHT * 0.45f);
	glm::vec2 place_pos(0);
	switch (att.att_placement) {
		case ATT_PLACEMENT::TOP: {
			place_pos = glm::vec2(0, offset.y);
			break;
		}
		case ATT_PLACEMENT::BOTTOM: {
			place_pos = glm::vec2(0, -offset.y);
			break;
		}
		case ATT_PLACEMENT::RIGHT: {
			place_pos = glm::vec2(offset.x, 0);
			break;
		}
		case ATT_PLACEMENT::LEFT: {
			place_pos = glm::vec2(-offset.x, 0);
			break;
		}
	}

	base_ext.transform_handle = create_transform(place_pos, go_globals.z_positions[BASE_EXT_Z_POS_KEY], glm::vec2(1), 0.f, 0.f, att.transform_handle);
	base_ext.quad_render_handle = create_quad_render(base_ext.transform_handle, create_color(240,74,94), base_extension_t::WIDTH, base_extension_t::HEIGHT, false, 0.f, -1);
	base_ext.rb_handle = create_rigidbody(base_ext.transform_handle, false, base_extension_t::WIDTH, base_extension_t::HEIGHT, true, PHYS_NONE, false, true);
	base_ext.attachment_handle = preview_state.preview_base_ext.attachment_handle;

	// attachment_t* att = get_attachment(base_ext.attachment_handle);
	// game_assert_msg(att, "attachment not found to make base ext");
	glm::vec2 hor_offset(base_extension_t::WIDTH * 0.45f, 0);
	glm::vec2 ver_offset(0, base_extension_t::HEIGHT * 0.45f);
	switch (type) {
		case TWO_ATT_HORIZONTAL: {
			base_ext.attachment_handles[0] = create_attachment(hor_offset, ATT_PLACEMENT::RIGHT, ATTMNT_GUN | ATTMNT_BASE_EXT, NULL, &base_ext);
			base_ext.attachment_handles[1] = create_attachment(-hor_offset, ATT_PLACEMENT::LEFT, ATTMNT_GUN | ATTMNT_BASE_EXT, NULL, &base_ext);
			base_ext.num_att_pts = 2;
			break;
		}
		case ONE_LEFT: {
			base_ext.attachment_handles[0] = create_attachment(-hor_offset, ATT_PLACEMENT::LEFT, ATTMNT_GUN | ATTMNT_BASE_EXT, NULL, &base_ext);
			base_ext.num_att_pts = 1;
			break;
		}
		case ONE_RIGHT: {
			base_ext.attachment_handles[0] = create_attachment(hor_offset, ATT_PLACEMENT::RIGHT, ATTMNT_GUN | ATTMNT_BASE_EXT, NULL, &base_ext);
			base_ext.num_att_pts = 1;
			break;
		}
		case THREE_ATT_LEFT_GONE: {
			base_ext.attachment_handles[0] = create_attachment(hor_offset, ATT_PLACEMENT::RIGHT, ATTMNT_GUN | ATTMNT_BASE_EXT, NULL, &base_ext);
			base_ext.attachment_handles[1] = create_attachment(-ver_offset, ATT_PLACEMENT::BOTTOM, ATTMNT_BASE_EXT, NULL, &base_ext);
			base_ext.attachment_handles[2] = create_attachment(ver_offset, ATT_PLACEMENT::TOP, ATTMNT_BASE_EXT, NULL, &base_ext);
			base_ext.num_att_pts = 3;
			break;
		}
		case THREE_ATT_RIGHT_GONE: {
			base_ext.attachment_handles[0] = create_attachment(-hor_offset, ATT_PLACEMENT::LEFT, ATTMNT_GUN | ATTMNT_BASE_EXT, NULL, &base_ext);
			base_ext.attachment_handles[1] = create_attachment(-ver_offset, ATT_PLACEMENT::BOTTOM, ATTMNT_BASE_EXT, NULL, &base_ext);
			base_ext.attachment_handles[2] = create_attachment(ver_offset, ATT_PLACEMENT::TOP, ATTMNT_BASE_EXT, NULL, &base_ext);
			base_ext.num_att_pts = 3;
			break;
		}
		case THREE_ATT_BOTTOM_GONE: {
			base_ext.attachment_handles[0] = create_attachment(-hor_offset, ATT_PLACEMENT::LEFT, ATTMNT_GUN | ATTMNT_BASE_EXT, NULL, &base_ext);
			base_ext.attachment_handles[1] = create_attachment(hor_offset, ATT_PLACEMENT::RIGHT, ATTMNT_GUN | ATTMNT_BASE_EXT, NULL, &base_ext);
			base_ext.attachment_handles[2] = create_attachment(ver_offset, ATT_PLACEMENT::TOP, ATTMNT_BASE_EXT, NULL, &base_ext);
			base_ext.num_att_pts = 3;
			break;
		}
	}
	
	for (int i = 0; i < base_ext.num_att_pts; i++)	 {
		attachment_t* att = get_attachment(base_ext.attachment_handles[i]);
		game_assert_msg(att, "att not found");
		transform_t* t = get_transform(att->transform_handle);
		att_pos_hash att_hash = hash_att_pos(t->global_position.x, t->global_position.y);
		attachment_t::overall_atts_placed_w_base_ext[att_hash] += 1;
		printf("for pos: (%f, %f), the value is now %i\n", t->global_position.x, t->global_position.y, attachment_t::overall_atts_placed_w_base_ext[att_hash]);
	}

	go_globals.attached_base_exts.push_back(base_ext);
	return base_ext.handle;
}

void update_base_ext(base_extension_t& attachment) {

}

void delete_base_ext(base_extension_t& attachment) {
	for (int i = 0; i < go_globals.attached_base_exts.size(); i++) {
		if (attachment.handle == go_globals.attached_base_exts[i].handle) {
			delete_transform(attachment.transform_handle);
			delete_rigidbody(attachment.rb_handle);
			delete_quad_render(attachment.quad_render_handle);

			for (int i = 0; i < attachment.num_att_pts; i++) {
				delete_attachment(attachment.attachment_handles[i]);
			}
			// delete_attachment(attachment.attachment_handles[1]);

			go_globals.attached_base_exts.erase(go_globals.attached_base_exts.begin() + i);
		}
	}	
}

const int gun_t::WIDTH = 80;
const int gun_t::HEIGHT = 20;
void create_attached_gun(int attachment_handle, bool facing_left, float fire_rate) {
	static int cnt = 0;

	gun_t gun;
	gun.handle = cnt++;
	gun.attachment_handle = preview_state.preview_gun.attachment_handle;

	attachment_t* att = get_attachment(gun.attachment_handle);
	game_assert_msg(att, "attachment not found to make gun");

	gun.fire_rate = fire_rate;
	gun.facing_left = facing_left;

	int multiplier = facing_left ? -1 : 1;
	glm::vec2 gun_pos = glm::vec2(multiplier * gun_t::WIDTH / 2.f, 0);

	gun.transform_handle = create_transform(gun_pos, go_globals.z_positions[GUN_Z_POS_KEY], glm::vec2(1), 0.f, 0.f, att->transform_handle);
	transform_t* g = get_transform(gun.transform_handle);
	gun.quad_render_handle = create_quad_render(gun.transform_handle, create_color(30,0,120), gun_t::WIDTH, gun_t::HEIGHT, false, 0.f, -1);
	gun.rb_handle = create_rigidbody(gun.transform_handle, false, gun_t::WIDTH, gun_t::HEIGHT, true, PHYS_NONE, true, true);

	if (gun.facing_left) {
		gun.prev_enemy_last_target_dir = glm::vec2(-1,0);
	} else {
		gun.prev_enemy_last_target_dir = glm::vec2(1,0);
	}
	gun.last_target_dir = gun.prev_enemy_last_target_dir;

	go_globals.attached_guns.push_back(gun);
}

std::vector<int> gun_t::enemy_died_handles;
const float gun_t::RETARGET_ANIM_TIME = 1;
const float gun_t::MAX_DISTANCE_TO_ENEMY = bullet_t::ALIVE_TIME * 800.f;
void update_attached_gun(gun_t& gun) {	
	set_quad_color(gun.quad_render_handle, create_color(60,0,240));

	transform_t* gun_transform = get_transform(gun.transform_handle);
	game_assert_msg(gun_transform, "transform for gun not found");

	if (gun.closest_enemy.handle != -1 && std::find(gun_t::enemy_died_handles.begin(), gun_t::enemy_died_handles.end(), gun.closest_enemy.handle) != gun_t::enemy_died_handles.end()) {
		gun.closest_enemy.handle = -1;
		gun.closest_enemy.transform_handle = -1;
	} else if (gun.closest_enemy.handle != -1) {
		transform_t* enemy_transform = get_transform(gun.closest_enemy.transform_handle);
		game_assert_msg(enemy_transform, "enemy transform not found");
		if (glm::distance(enemy_transform->global_position, gun_transform->global_position) > gun_t::MAX_DISTANCE_TO_ENEMY) {
			gun.closest_enemy.handle = -1;
			gun.closest_enemy.transform_handle = -1;
		}
	}

	attachment_t* att = get_attachment(gun.attachment_handle);
	game_assert_msg(att, "att for attached gun not found");
	transform_t* attachment_transform = get_transform(att->transform_handle);
	game_assert_msg(attachment_transform, "transform for gun's attach pt not found");
	
	if (gun.closest_enemy.handle == -1) {
		float closest = FLT_MAX;
		for (int i = 0; i < go_globals.enemies.size(); i++) {
			transform_t* enemy_transform = get_transform(go_globals.enemies[i].transform_handle);
			game_assert_msg(enemy_transform, "enemy transform not found");
			float distance = glm::distance(enemy_transform->global_position, attachment_transform->global_position);
			if (distance < closest && distance <= gun_t::MAX_DISTANCE_TO_ENEMY && std::find(gun_t::enemy_died_handles.begin(), gun_t::enemy_died_handles.end(), go_globals.enemies[i].handle) == gun_t::enemy_died_handles.end()) {
				closest = distance;
				gun.closest_enemy.handle = go_globals.enemies[i].handle;
				gun.closest_enemy.transform_handle = go_globals.enemies[i].transform_handle;
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
		glm::vec2 pos(attachment_transform->global_position.x, attachment_transform->global_position.y);
		// create_bullet(pos, glm::vec2(gun_facing_dir.x, gun_facing_dir.y), 800.f);
	}

	gun.last_target_dir = gun_facing_dir;
}

void delete_attached_gun(gun_t& gun) {
	for (int i = 0; i < go_globals.attached_guns.size(); i++) {
		if (gun.handle == go_globals.attached_guns[i].handle) {
			delete_transform(gun.transform_handle);
			delete_rigidbody(gun.rb_handle);
			delete_quad_render(gun.quad_render_handle);
			go_globals.attached_guns.erase(go_globals.attached_guns.begin() + i);
		}
	}
}

const float bullet_t::ALIVE_TIME = 1.f;
const int bullet_t::WIDTH = 8;
const int bullet_t::HEIGHT = 8;

void create_bullet(glm::vec2 start_pos, glm::vec2 move_dir, float speed) {
	static int cnt = 0;
	bullet_t bullet;
	bullet.handle = cnt++;

	bullet.transform_handle = create_transform(start_pos, go_globals.z_positions[BULLET_Z_POS_KEY], glm::vec2(1), 0, 0, -1);
	bullet.quad_render_handle = create_quad_render(bullet.transform_handle, glm::vec3(1,1,0), bullet_t::WIDTH, bullet_t::HEIGHT, false, 0, -1);
	bullet.rb_handle = create_rigidbody(bullet.transform_handle, false, bullet_t::WIDTH, bullet_t::HEIGHT, true, PHYS_BULLET, true, true);

	bullet.dir = move_dir;
	bullet.speed = speed;
	bullet.creation_time = game::time_t::cur_time;
	go_globals.bullets.push_back(bullet);
}

void mark_bullet_for_deletion(int bullet_handle) {
	bullets_to_delete.push_back(bullet_handle);
}

void update_bullet(bullet_t& bullet) {
	transform_t* bullet_transform = get_transform(bullet.transform_handle);
	game_assert_msg(bullet_transform, "bullet transform not found");
	glm::vec2 displace = bullet.dir * glm::vec2(bullet.speed * game::time_t::delta_time);
	bullet_transform->global_position.x += displace.x;
	bullet_transform->global_position.y += displace.y;

	std::vector<kin_w_kin_col_t> cols = get_from_kin_w_kin_cols(bullet.rb_handle, PHYS_BULLET);
	for (kin_w_kin_col_t& col : cols) {
		if (col.kin_type1 == PHYS_ENEMY || col.kin_type2 == PHYS_ENEMY) {
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
	bullet_t& bullet = go_globals.bullets[idx];
	if (bullet.handle == handle) {
		delete_transform(bullet.transform_handle);
		delete_quad_render(bullet.quad_render_handle);
		delete_rigidbody(bullet.rb_handle);
		go_globals.bullets.erase(go_globals.bullets.begin() + idx);
		return;
	}
}

void delete_bullet(bullet_t& bullet) {
	for (int i = 0; i < go_globals.bullets.size(); i++) {
		if (go_globals.bullets[i].handle == bullet.handle) {
			delete_transform(bullet.transform_handle);
			delete_quad_render(bullet.quad_render_handle);
			delete_rigidbody(bullet.rb_handle);
			go_globals.bullets.erase(go_globals.bullets.begin() + i);
			return;
		}
	}
}

const int enemy_bullet_t::WIDTH = 8;
const int enemy_bullet_t::HEIGHT = 8;
const float enemy_bullet_t::ALIVE_TIME = 1.f;
void create_enemy_bullet(glm::vec2 pos, glm::vec2 dir, float speed) {
	static int cnt = 0;
	enemy_bullet_t enemy_bullet;
	enemy_bullet.handle = cnt++;
	enemy_bullet.transform_handle = create_transform(pos, go_globals.z_positions[ENEMY_BULLET_Z_POS_KEY], glm::vec2(1), 0, 0, -1);
	enemy_bullet.rb_handle = create_rigidbody(enemy_bullet.transform_handle, false, enemy_bullet_t::WIDTH, enemy_bullet_t::HEIGHT, true, PHYS_ENEMY_BULLET, true, true);
	enemy_bullet.quad_render_handle = create_quad_render(enemy_bullet.transform_handle, glm::vec3(0,1,0), enemy_bullet_t::WIDTH, enemy_bullet_t::HEIGHT, false, 0.f, -1);
	enemy_bullet.creation_time = game::time_t::game_cur_time;
	enemy_bullet.speed = speed;
	enemy_bullet.dir = dir;
	go_globals.enemy_bullets.push_back(enemy_bullet);
}

static void mark_enemy_bullet_for_deletion(int handle) {
	enemy_bullets_to_delete.push_back(handle);
}

void update_enemy_bullet(enemy_bullet_t& bullet) {
	transform_t* bullet_transform = get_transform(bullet.transform_handle);
	game_assert_msg(bullet_transform, "bullet transform not found");
	glm::vec2 displace = bullet.dir * glm::vec2(bullet.speed * game::time_t::delta_time);
	bullet_transform->global_position.x += displace.x;
	bullet_transform->global_position.y += displace.y;

	std::vector<kin_w_kin_col_t> cols = get_from_kin_w_kin_cols(bullet.rb_handle, PHYS_ENEMY_BULLET);
	for (kin_w_kin_col_t& col : cols) {
		if (col.kin_type1 == PHYS_BASE || col.kin_type2 == PHYS_BASE) {
			mark_enemy_bullet_for_deletion(bullet.handle);
			return;
		}
	}

	if (bullet.creation_time + bullet_t::ALIVE_TIME < game::time_t::game_cur_time) {
		mark_enemy_bullet_for_deletion(bullet.handle);
	}
}

static void delete_enemy_bullet_by_index(int idx, const int enemy_bullet_handle) {
	enemy_bullet_t& bullet = go_globals.enemy_bullets[idx];
	if (bullet.handle == enemy_bullet_handle) {
		delete_transform(bullet.transform_handle);
		delete_quad_render(bullet.quad_render_handle);
		delete_rigidbody(bullet.rb_handle);
		go_globals.enemy_bullets.erase(go_globals.enemy_bullets.begin() + idx);
	}
}

void delete_enemy_bullet(enemy_bullet_t& bullet) {
	for (int i = 0; i < go_globals.enemy_bullets.size(); i++) {
		if (go_globals.enemy_bullets[i].handle == bullet.handle) {
			delete_transform(bullet.transform_handle);
			delete_quad_render(bullet.quad_render_handle);
			delete_rigidbody(bullet.rb_handle);
			go_globals.enemy_bullets.erase(go_globals.enemy_bullets.begin() + i);
			return;
		}
	}
}

static int enemy_spawner_cnt = 0;
const float enemy_spawner_t::TIME_BETWEEN_SPAWNS = 2.5f;
void create_enemy_spawner(glm::vec2 pos, MOVE_DIR dir) {
	enemy_spawner_t enemy_spawner;	
	enemy_spawner.handle = enemy_spawner_cnt++;
	enemy_spawner.enemy_type_spawner = ENEMY_TYPE::GROUND;
	enemy_spawner.transform_handle = create_transform(pos, go_globals.z_positions[ENEMY_SPAWNER_Z_POS_KEY], glm::vec2(1), 0, 0, -1);
	enemy_spawner.quad_render_handle = create_quad_render(enemy_spawner.transform_handle, create_color(25,25,75), 60, 140, false, 0, -1);
	enemy_spawner.dir = dir;

	go_globals.enemy_spawners.push_back(enemy_spawner);
}

void create_air_enemy_spawner(glm::vec2 pos) {
	enemy_spawner_t enemy_spawner;	
	enemy_spawner.handle = enemy_spawner_cnt++;
	enemy_spawner.enemy_type_spawner = ENEMY_TYPE::AIR;
	enemy_spawner.transform_handle = create_transform(pos, go_globals.z_positions[ENEMY_SPAWNER_Z_POS_KEY], glm::vec2(1), 0, 0, -1);
	enemy_spawner.quad_render_handle = create_quad_render(enemy_spawner.transform_handle, create_color(25,75,75), 60, 60, false, 0, -1);
	enemy_spawner.dir = MOVE_DIR::NONE;

	go_globals.enemy_spawners.push_back(enemy_spawner);
}

void update_enemy_spawner(enemy_spawner_t& spawner) {
	spawner.enemy_relative_time += game::time_t::delta_time;
	if (spawner.last_spawn_time + enemy_spawner_t::TIME_BETWEEN_SPAWNS >= spawner.enemy_relative_time) return;
	transform_t* transform = get_transform(spawner.transform_handle);
	game_assert_msg(transform, "transform for enemy spawner not found");
	glm::vec2 pos = transform->global_position;
	if (spawner.enemy_type_spawner == ENEMY_TYPE::GROUND) {
		static bool made = false;
		create_enemy(pos, spawner.dir, 40.f);
	} else if (spawner.enemy_type_spawner == ENEMY_TYPE::AIR) {
		create_air_enemy(pos, 40.f);
	}
	spawner.last_spawn_time = spawner.enemy_relative_time;
}

void update_score() {
    set_ui_value(std::string("num_enemies_left_to_kill"), std::to_string(go_globals.score.enemies_left_to_kill));
	if (go_globals.score.enemies_left_to_kill == 0) {
		globals.scene_manager.queue_level_load = true;
		if (globals.scene_manager.cur_level == globals.scene_manager.levels.size()) {
			globals.scene_manager.level_to_load = GAME_OVER_SCREEN_LEVEL;
		} else {
			globals.scene_manager.level_to_load = LEVELS_DISPLAY;
			unlock_level(globals.scene_manager.cur_level + 1);
			// globals.scene_manager.level_to_load = globals.scene_manager.cur_level + 1;
		}
	}
}

void gos_update() {
    update_preview_mode(); 	

	for (attachment_t& att : go_globals.attachments) {
		update_attachment(att);
	}

	for (bullet_t& bullet : go_globals.bullets) {
		update_bullet(bullet);
	}	

	for (enemy_spawner_t& enemy_spawner : go_globals.enemy_spawners) {
		update_enemy_spawner(enemy_spawner);
	}

	create_absolute_panel("enemies_health_panel", go_globals.z_positions[ENEMY_Z_POS_KEY]);
	for (enemy_t& enemy : go_globals.enemies) {
		update_enemy(enemy);
	}
	enemy_t::deleted_base_handles.clear();
	end_absolute_panel();

	for (enemy_bullet_t& enemy_bullet : go_globals.enemy_bullets) {
		update_enemy_bullet(enemy_bullet);
	}

	for (gun_t& gun : go_globals.attached_guns) {
		update_attached_gun(gun);
	}
	gun_t::enemy_died_handles.clear();

	create_absolute_panel("base_health_panel", go_globals.z_positions[BASE_Z_POS_KEY]);
	for (base_t& base : go_globals.gun_bases) {
		update_base(base);
	}
	end_absolute_panel();

	update_hierarchy_based_on_globals();
	update_score();

	gos_update_delete_pass();
}

void gos_update_delete_pass() {
	for (int handle : bullets_to_delete) {
		for (int i = 0; i < go_globals.bullets.size(); i++) {
			if (go_globals.bullets[i].handle == handle) {
				delete_bullet_by_index(i, handle);
			}
		}
	}
	bullets_to_delete.clear();

	for (int handle : enemy_bullets_to_delete) {
		for (int i = 0; i < go_globals.enemy_bullets.size(); i++) {
			if (go_globals.enemy_bullets[i].handle == handle) {
				delete_enemy_bullet_by_index(i, handle);
			}
		}
	}
	enemy_bullets_to_delete.clear();

	for (int handle : bases_to_delete) {
		for (int i = 0; i < go_globals.gun_bases.size(); i++) {
			if (go_globals.gun_bases[i].handle == handle) {
				delete_base_by_index(i, handle);
			}
		}
	}
	bases_to_delete.clear();

	for (int handle : go_globals.enemies_to_delete) {
		for (int i = 0; i < go_globals.enemies.size(); i++) {
			if (go_globals.enemies[i].handle == handle) {
				delete_enemy_by_index(i, handle);
			}
		}
	}
	go_globals.enemies_to_delete.clear();
}

void delete_gos() {
	go_globals.attachments.clear();
	attachment_t::overall_atts_placed_w_base_ext.clear();

	go_globals.attached_guns.clear();
	go_globals.bullets.clear();
	go_globals.enemy_spawners.clear();
	go_globals.enemies.clear();
	go_globals.	gun_bases.clear();
	go_globals.attached_base_exts.clear();
	go_globals.enemy_bullets.clear();

	preview_state.preview_base = base_t();
	preview_state.preview_base_ext = base_extension_t();
	preview_state.preview_gun = gun_t();
}

std::unordered_map<att_pos_hash, int> attachment_t::overall_atts_placed_w_base_ext;

att_pos_hash hash_att_pos(float x, float y) {
	// return (att_pos_hash)(std::hash<int>()(x) ^ std::hash<int>()(y));
	return (att_pos_hash)(static_cast<int>(x) ^ static_cast<int>(y));
}