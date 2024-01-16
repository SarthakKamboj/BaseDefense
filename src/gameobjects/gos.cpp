#include "gos.h"

#include "gfx/quad.h"
#include "utils/general.h"
#include "physics/physics.h"
#include "constants.h"
#include "utils/time.h"
#include "globals.h"

#include <vector>

extern globals_t globals;

static std::vector<base_t> gun_bases;
static std::vector<base_extension_t> attached_base_exts;
static std::vector<gun_t> attached_guns;
static std::vector<bullet_t> bullets;
static std::vector<attachment_t> attachments;

static std::vector<enemy_spawner_t> enemy_spawners;
static std::vector<enemy_t> enemies;

static gun_t preview_gun;
static base_extension_t preview_base_ext;
static base_t preview_base;
PREVIEW_MODE preview_mode = PREVIEW_MODE::PREVIEW_BASE;

void init_preview() {
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
	if (preview_mode != PREVIEW_MODE::PREVIEW_BASE) return;

	quad_render_t* quad_render = get_quad_render(preview_base.quad_render_handle);
	game_assert_msg(quad_render, "quad render for base not found");	

	glm::vec2 mouse = mouse_to_world_pos();
	transform_t* preview_transform = get_transform(preview_base.transform_handle);
	game_assert_msg(preview_transform, "could not find transform for preview base");

	preview_transform->global_position = glm::vec3(mouse.x, base_t::HEIGHT * 0.5f, 0.f);

	if (globals.window.user_input.left_mouse_release) {
		create_base(preview_transform->global_position);
	}

	bool left_down = globals.window.user_input.left_mouse_down;
	quad_render->render = left_down;

	update_hierarchy_based_on_globals();
}

void create_base(glm::vec3 pos) {
	static int cnt = 0;
	base_t gun_base;
	gun_base.handle = cnt++;

	gun_base.transform_handle = create_transform(pos, glm::vec3(1), 0.f, 0.f);
	gun_base.quad_render_handle = create_quad_render(gun_base.transform_handle, create_color(59,74,94), base_t::WIDTH, base_t::HEIGHT, false, 0.f, -1);
	gun_base.rb_handle = create_rigidbody(gun_base.transform_handle, false, base_t::WIDTH, base_t::HEIGHT, true, PHYS_NONE, true, true);

	gun_base.previewing = false;

	// gun_base.attach_pt_transform_handles[0] = create_transform(pos - glm::vec3(base_t::WIDTH * 0.4f, 0, 0), glm::vec3(1), 0, 0, -1);
	// gun_base.attach_pt_transform_handles[1] = create_transform(pos + glm::vec3(base_t::WIDTH * 0.4f, 0, 0), glm::vec3(1), 0, 0, -1);
	// gun_base.attach_pt_transform_handles[2] = create_transform(pos + glm::vec3(0, base_t::HEIGHT * 0.4f, 0), glm::vec3(1), 0, 0, -1);

	// gun_base.attachment_handles[0] = create_attachment(pos - glm::vec3(base_t::WIDTH * 0.4f, 0, 0), true, ATTMNT_GUN | ATTMNT_BASE_EXT);
	// gun_base.attachment_handles[1] = create_attachment(pos + glm::vec3(base_t::WIDTH * 0.4f, 0, 0), false, ATTMNT_GUN | ATTMNT_BASE_EXT);
	// gun_base.attachment_handles[2] = create_attachment(pos + glm::vec3(0, base_t::HEIGHT * 0.4f, 0), false, ATTMNT_BASE_EXT);

	gun_base.attachment_handles[0] = create_attachment(glm::vec3(-base_t::WIDTH * 0.4f, 0, 0), true, ATTMNT_GUN | ATTMNT_BASE_EXT, &gun_base, NULL);
	gun_base.attachment_handles[1] = create_attachment(glm::vec3(base_t::WIDTH * 0.4f, 0, 0), false, ATTMNT_GUN | ATTMNT_BASE_EXT, &gun_base, NULL);
	gun_base.attachment_handles[2] = create_attachment(glm::vec3(0, base_t::HEIGHT * 0.4f, 0), false, ATTMNT_BASE_EXT, &gun_base, NULL);

	gun_bases.push_back(gun_base);
}

void update_base(base_t& base) {

	// if (preview_mode != PREVIEW_MODE::PREVIEW_BASE_EXT) return;

	// glm::vec3 mouse(globals.window.user_input.mouse_x, globals.window.user_input.mouse_y, 0);

	// for (int i = 0; i < NUM_BASE_ATTACH_PTS; i++) {
	// 	transform_t* transform = get_transform(base.attach_pt_transform_handles[i]);
	// 	game_assert_msg(transform, "could not find transform of base's attachment pt");
	// 	bool attached = base.attach_pts_attached[i];
	// 	if (!attached && glm::distance(mouse, transform->position) <= 20) {
	// 		add_debug_pt(transform->position);

	// 		transform_t* preview_transform = get_transform(preview_base_ext.transform_handle);
	// 		game_assert_msg(preview_transform, "transform of preview attachment not found");

	// 		preview_base_ext.free = false;
	// 		preview_base_ext.index_attached_to_on_base = i;
	// 		preview_transform->position = transform->position;
	// 	}
	// }
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
	transform_t* transform = get_transform(attachment.transform_handle);
	game_assert_msg(transform, "could not find transform of attachment pt");

	if (preview_mode == PREVIEW_MODE::PREVIEW_GUN && (attachment.attachment_types & ATTACHMENT_TYPE::ATTMNT_GUN) == 0) return;
	if (preview_mode == PREVIEW_MODE::PREVIEW_BASE_EXT && (attachment.attachment_types & ATTACHMENT_TYPE::ATTMNT_BASE_EXT) == 0) return;

	glm::vec2 mouse = mouse_to_world_pos();
	glm::vec3 mouse3 = glm::vec3(mouse.x, mouse.y, 0);
	if (glm::distance(mouse3, transform->global_position) > 20) return;
	transform_t* preview_transform = preview_mode == PREVIEW_MODE::PREVIEW_GUN ? get_transform(preview_gun.transform_handle) : get_transform(preview_base_ext.transform_handle);
	if (preview_mode == PREVIEW_MODE::PREVIEW_GUN) {
		transform_t* gun_transform = get_transform(preview_gun.transform_handle);
		game_assert_msg(gun_transform, "transform of preview gun not found");
		preview_gun.free = false;
		preview_gun.attachment_handle = attachment.handle;
		gun_transform->global_position = transform->global_position;
	} else if (preview_mode == PREVIEW_MODE::PREVIEW_BASE_EXT) {
		transform_t* preview_transform = get_transform(preview_base_ext.transform_handle);
		game_assert_msg(preview_transform, "transform of preview attachment not found");
		preview_base_ext.attachment_handle = attachment.handle;
		preview_base_ext.free = false;
		preview_transform->global_position = transform->global_position;
	}

	update_hierarchy_based_on_globals();
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

	// attachment.left_attach_pt_transform_handle = create_transform(pos - offset, glm::vec3(1), 0.f, 0.f);
	// attachment.right_attach_pt_transform_handle = create_transform(pos + offset, glm::vec3(1), 0.f, 0.f);

	// gun_bases[0].attach_pts_attached[preview_base_ext.index_attached_to_on_base] = true;x

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

	// if (preview_mode != PREVIEW_MODE::PREVIEW_GUN) return;

	// transform_t* left_transform = get_transform(attachment.left_attach_pt_transform_handle);
	// game_assert_msg(left_transform, "left transform for this attachment is not valid");
	// transform_t* right_transform = get_transform(attachment.right_attach_pt_transform_handle);
	// game_assert_msg(right_transform, "right transform for this attachment is not valid");

	// glm::vec3 mouse(globals.window.user_input.mouse_x, globals.window.user_input.mouse_y, 0);
	// if (!attachment.left_attached && glm::distance(mouse, left_transform->position) <= 20) {
	// 	add_debug_pt(left_transform->position);
	// 	preview_gun.free = false;
	// 	preview_gun.attachment_handle = attachment.handle;
	// 	preview_gun.left_attached = true;
	// 	transform_t* gun_transform = get_transform(preview_gun.transform_handle);
	// 	game_assert_msg(gun_transform, "transform of preview gun not found");
	// 	gun_transform->position = left_transform->position;
	// } else if (!attachment.right_attached && glm::distance(mouse, right_transform->position) <= 20) {
	// 	add_debug_pt(right_transform->position);
	// 	preview_gun.free = false;
	// 	preview_gun.attachment_handle = attachment.handle;
	// 	preview_gun.left_attached = false;
	// 	transform_t* gun_transform = get_transform(preview_gun.transform_handle);
	// 	game_assert_msg(gun_transform, "transform of preview gun not found");
	// 	gun_transform->position = right_transform->position;
	// }
}

void init_base_ext_preview() {
	preview_base_ext.handle = -1;

	preview_base_ext.transform_handle = create_transform(glm::vec3(0), glm::vec3(1), 0.f, 0.f);
	preview_base_ext.quad_render_handle = create_quad_render(preview_base_ext.transform_handle, create_color(145, 145, 145), base_extension_t::WIDTH, base_extension_t::HEIGHT, false, 0.f, -1);
	preview_base_ext.rb_handle = create_rigidbody(preview_base_ext.transform_handle, false, base_extension_t::WIDTH, base_extension_t::HEIGHT, true, PHYS_NONE, true, false);
}

void update_preview_base_ext() {
	bool left_down = globals.window.user_input.left_mouse_down;
	bool left_release = globals.window.user_input.left_mouse_release;

	quad_render_t* preview_quad = get_quad_render(preview_base_ext.quad_render_handle);
	game_assert_msg(preview_quad, "quad render for preview attachment not found");

	if (preview_mode != PREVIEW_MODE::PREVIEW_BASE_EXT) {
		preview_quad->render = false;
		return;
	}

	preview_quad->render = left_down;

	transform_t* transform = get_transform(preview_base_ext.transform_handle);
	game_assert_msg(transform, "transform of preview attachment not found");
	if(left_release && !preview_base_ext.free) {
		preview_base_ext.free = true;
		create_base_ext(transform->global_position);
		return;
	}	

	if (!left_down) {	
		return;
	}

	glm::vec2 mouse = mouse_to_world_pos();
	if (preview_base_ext.free) {
		transform->global_position = glm::vec3(mouse.x, mouse.y, 0);
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
	preview_gun.quad_render_handle = create_quad_render(preview_gun.transform_handle, create_color(45,45,45), gun_t::WIDTH, gun_t::HEIGHT, false, 0.f, -1);
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

	attached_guns.push_back(gun);
}

void update_preview_gun() {
	bool left_down = globals.window.user_input.left_mouse_down;
	bool left_release = globals.window.user_input.left_mouse_release;

	quad_render_t* preview_quad = get_quad_render(preview_gun.quad_render_handle);
	game_assert_msg(preview_quad, "quad render for preview gun not found");

	if (preview_mode != PREVIEW_MODE::PREVIEW_GUN) {
		preview_quad->render = false;
		return;
	}

	preview_quad->render = left_down;

	if(left_release && !preview_gun.free) {
		attachment_t* att = get_attachment(preview_gun.attachment_handle);
		game_assert_msg(att, "attachment to create gun not found");
		create_attached_gun(preview_gun.attachment_handle, att->facing_left, 2.f);
		preview_gun.free = true;
		return;
	}	

	if (!left_down) {	
		return;
	}

	if (preview_gun.free) {
		transform_t* transform = get_transform(preview_gun.transform_handle);
		game_assert_msg(transform, "transform of preview gun not found");
		glm::vec2 mouse = mouse_to_world_pos();
		transform->global_position = glm::vec3(mouse.x, mouse.y, 0);
	}
}

void update_attached_gun(gun_t& gun) {	
	set_quad_color(gun.quad_render_handle, create_color(60,0,240));

	transform_t* gun_transform = get_transform(gun.transform_handle);
	game_assert_msg(gun_transform, "transform for gun not found");

	attachment_t* att = get_attachment(gun.attachment_handle);
	game_assert_msg(att, "att for attached gun not found");
	transform_t* attachment_transform = get_transform(att->transform_handle);
	game_assert_msg(attachment_transform, "transform for gun's attach pt not found");
	
	int mouse_x = globals.window.user_input.mouse_x;
	int mouse_y = globals.window.user_input.mouse_y;

	glm::vec2 to_mouse = glm::normalize(glm::vec2(mouse_x - attachment_transform->global_position.x, mouse_y - attachment_transform->global_position.y));
	glm::vec2 dir_facing = glm::vec2(-1, 0);
	glm::vec2 dir_perpen = glm::vec2(0, -1);
	if (!gun.facing_left) {
		dir_facing = glm::vec2(1, 0);
		dir_perpen = glm::vec2(0, 1);
	}
	float cos_z = glm::dot(to_mouse, dir_facing);
	float z_rad = acos(cos_z);
	glm::vec3 rot = attachment_transform->local_rotation;
	rot.z = glm::degrees(z_rad);

	float below_gun = glm::dot(to_mouse, dir_perpen);
	if (below_gun < 0) {
		rot.z *= -1;
	}
	set_local_rot(attachment_transform, rot);

	float time_between_fires = 1 / gun.fire_rate;
	if (gun.time_since_last_fire + time_between_fires < game::time_t::cur_time) {
		gun.time_since_last_fire = game::time_t::cur_time;
		create_bullet(attachment_transform->global_position, glm::vec3(to_mouse.x, to_mouse.y, 0), 800.f);
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

void update_bullet(bullet_t& bullet) {
	transform_t* bullet_transform = get_transform(bullet.transform_handle);
	game_assert_msg(bullet_transform, "bullet transform not found");
	bullet_transform->global_position += bullet.dir * glm::vec3(bullet.speed * game::time_t::delta_time);

	std::vector<kin_w_kin_col_t> cols = get_from_kin_w_kin_cols(bullet.rb_handle, PHYS_BULLET);
	for (kin_w_kin_col_t& col : cols) {
		if (col.kin_type1 == PHYS_ENEMY || col.kin_type2 == PHYS_ENEMY) {
			delete_bullet(bullet);
			return;
		}
	}

	if (bullet.creation_time + bullet_t::ALIVE_TIME < game::time_t::cur_time) {
		delete_bullet(bullet);
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

const int enemy_t::HEIGHT = 80;
const int enemy_t::WIDTH = 40;
void create_enemy(glm::vec3 pos, int dir, float speed) {
	static int cnt = 0;
	enemy_t enemy;
	enemy.handle = cnt++;
	enemy.dir = dir;
	enemy.speed = speed;
	enemy.transform_handle = create_transform(pos, glm::vec3(1), 0, 0, -1);
	enemy.quad_render_handle = create_quad_render(enemy.transform_handle, create_color(75, 25, 25), enemy_t::WIDTH, enemy_t::HEIGHT, false, 0, -1);
	enemy.rb_handle = create_rigidbody(enemy.transform_handle, false, enemy_t::WIDTH, enemy_t::HEIGHT, true, PHYS_ENEMY, true, true);
	enemies.push_back(enemy);
}

void update_enemy(enemy_t& enemy) {
	transform_t* transform = get_transform(enemy.transform_handle);
	game_assert_msg(transform, "transform for enemy not found");
	transform->global_position += glm::vec3(enemy.speed * enemy.dir * game::time_t::delta_time,0,0);

	std::vector<kin_w_kin_col_t> cols = get_from_kin_w_kin_cols(enemy.rb_handle, PHYS_ENEMY);
	for (kin_w_kin_col_t& col : cols) {
		if (col.kin_type1 == PHYS_BULLET || col.kin_type2 == PHYS_BULLET) {
			enemy.health -= 50;
			if (enemy.health <= 0) {
				delete_enemy(enemy.handle);
			}
		}
	}

}

void delete_enemy(int enemy_handle) {
	for (int i = 0; i < enemies.size(); i++) {
		if (enemies[i].handle == enemy_handle) {
			delete_transform(enemies[i].transform_handle);
			delete_rigidbody(enemies[i].rb_handle);
			delete_quad_render(enemies[i].quad_render_handle);
			enemies.erase(enemies.begin() + i);
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
	if (spawner.last_spawn_time + enemy_spawner_t::TIME_BETWEEN_SPAWNS >= game::time_t::cur_time) return;
	transform_t* transform = get_transform(spawner.transform_handle);
	game_assert_msg(transform, "transform for enemy spawner not found");
	create_enemy(transform->global_position, 1, 40.f);
	spawner.last_spawn_time = game::time_t::cur_time;
}

void gos_update() {

	if (globals.window.user_input.s_pressed) {
		if (preview_mode == PREVIEW_MODE::PREVIEW_GUN) {
			preview_mode = PREVIEW_MODE::PREVIEW_BASE_EXT;
		} else if (preview_mode == PREVIEW_MODE::PREVIEW_BASE_EXT) {
			preview_mode = PREVIEW_MODE::PREVIEW_BASE;
		} else if (preview_mode == PREVIEW_MODE::PREVIEW_BASE) {
			preview_mode = PREVIEW_MODE::PREVIEW_GUN;
		}
	}	

	update_preview_base();

	preview_base_ext.free = true;
	preview_gun.free = true;

	for (attachment_t& att : attachments) {
		update_attachment(att);
	}

	update_preview_gun();
	update_preview_base_ext();

	for (gun_t& gun : attached_guns) {
		update_attached_gun(gun);
	}

	for (bullet_t& bullet : bullets) {
		update_bullet(bullet);
	}

	for (enemy_spawner_t& enemy_spawner : enemy_spawners) {
		update_enemy_spawner(enemy_spawner);
	}

	for (enemy_t& enemy : enemies) {
		update_enemy(enemy);
	}

	update_hierarchy_based_on_globals();
}