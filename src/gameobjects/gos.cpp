#include "gos.h"

#include "gfx/quad.h"
#include "utils/general.h"
#include "physics/physics.h"
#include "constants.h"
#include "utils/time.h"
#include "globals.h"

#include <vector>

extern globals_t globals;

std::vector<base_t> gun_bases;
std::vector<base_attachment_t> attachments;
std::vector<gun_t> guns;

const int base_t::WIDTH = 100;
const int base_t::HEIGHT = 300;

void create_base() {
	static int cnt = 0;
	base_t gun_base;
	gun_base.handle = cnt++;

	gun_base.transform_handle = create_transform(glm::vec3(400, 200, 0), glm::vec3(1), 0.f, 0.f);
	gun_base.quad_render_handle = create_quad_render(gun_base.transform_handle, create_color(59,74,94), base_t::WIDTH, base_t::HEIGHT, false, 0.f, -1);
	gun_base.rb_handle = create_rigidbody(gun_base.transform_handle, false, base_t::WIDTH, base_t::HEIGHT, true, PHYSICS_RB_TYPE::NONE, true, true);

	gun_bases.push_back(gun_base);
}

const int base_attachment_t::WIDTH = 100;
const int base_attachment_t::HEIGHT = 40;
int create_base_attachment() {
	static int cnt = 0;
	base_attachment_t attachment;
	attachment.handle = cnt++;

	glm::vec3 pos = glm::vec3(400, 370, 0);
	attachment.transform_handle = create_transform(pos, glm::vec3(1), 0.f, 0.f);
	attachment.quad_render_handle = create_quad_render(attachment.transform_handle, create_color(240,74,94), base_attachment_t::WIDTH, base_attachment_t::HEIGHT, false, 0.f, -1);
	attachment.rb_handle = create_rigidbody(attachment.transform_handle, false, base_attachment_t::WIDTH, base_attachment_t::HEIGHT, true, PHYSICS_RB_TYPE::NONE, true, true);	

	glm::vec3 offset(base_attachment_t::WIDTH * 0.4, 0, 0);
	attachment.left_attach_pt_transform_handle = create_transform(pos - offset, glm::vec3(1), 0.f, 0.f);
	attachment.right_attach_pt_transform_handle = create_transform(pos + offset, glm::vec3(1), 0.f, 0.f);

	add_debug_pt(pos + offset);
	add_debug_pt(pos - offset);

	attachments.push_back(attachment);
	return attachment.handle;
}

base_attachment_t* get_base_attachment(int handle) {
	for (int i = 0; i < attachments.size(); i++) {
		if (handle == attachments[i].handle) {
			return &attachments[i];
		}
	}
	return NULL;
}

const int gun_t::WIDTH = 80;
const int gun_t::HEIGHT = 20;
void create_gun(int base_attachment_handle, bool left_attached) {
	static int cnt = 0;

	base_attachment_t* attachment = get_base_attachment(base_attachment_handle);
	game_assert_msg(attachment, "base attachment not valid not this gun");

	gun_t gun;
	gun.handle = cnt++;

	gun.base_attachment_handle = base_attachment_handle;
	gun.left_attached = left_attached;

	int transform_handle = left_attached ? attachment->left_attach_pt_transform_handle : attachment->right_attach_pt_transform_handle;
	gun.attachment_transform_handle = transform_handle;

	transform_t* attachment_transform = get_transform(transform_handle);
	game_assert_msg(attachment_transform, "attachment position not valid");

	int multiplier = left_attached ? -1 : 1;
	glm::vec3 gun_pos = glm::vec3(multiplier * gun_t::WIDTH / 2.f, 0, 0);

	gun.transform_handle = create_transform(gun_pos, glm::vec3(1), 0.f, 0.f, transform_handle);
	gun.quad_render_handle = create_quad_render(gun.transform_handle, create_color(60,0,240), gun_t::WIDTH, gun_t::HEIGHT, false, 0.f, -1);
	gun.rb_handle = create_rigidbody(gun.transform_handle, false, gun_t::WIDTH, gun_t::HEIGHT, true, PHYSICS_RB_TYPE::NONE, true, true);

	guns.push_back(gun);
}

void update_gun(gun_t& gun) {
	transform_t* attachment_transform = get_transform(gun.attachment_transform_handle);
	int mouse_x = globals.window.user_input.mouse_x;
	int mouse_y = globals.window.user_input.mouse_y;

	glm::vec2 to_mouse = glm::normalize(glm::vec2(mouse_x - attachment_transform->position.x, mouse_y - attachment_transform->position.y));
	glm::vec2 dir_facing = glm::vec2(-1, 0);
	glm::vec2 dir_perpen = glm::vec2(0, -1);
	if (!gun.left_attached) {
		dir_facing = glm::vec2(1, 0);
		dir_perpen = glm::vec2(0, 1);
	}
	float cos_z = glm::dot(to_mouse, dir_facing);
	float z_rad = acos(cos_z);
	attachment_transform->rotation.z = glm::degrees(z_rad);

	float below_gun = glm::dot(to_mouse, dir_perpen);
	if (below_gun < 0) {
		attachment_transform->rotation.z *= -1;		
	}

	// const float angular_vel = 180.f; 
	// attachment_transform->rotation.z += angular_vel * game::time_t::delta_time;
}

void gos_update() {
	for (gun_t& gun : guns) {
		update_gun(gun);
	}
}