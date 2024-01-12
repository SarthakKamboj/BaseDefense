#include "gos.h"

#include "gfx/quad.h"
#include "utils/general.h"
#include "physics/physics.h"

#include <vector>

std::vector<gun_base_t> gun_bases;

const int gun_base_t::WIDTH = 100;
const int gun_base_t::HEIGHT = 300;

void create_gun_base() {
	static int cnt = 0;
	gun_base_t gun_base;
	gun_base.handle = cnt++;
	int transform_handle = create_transform(glm::vec3(400, 200, 0), glm::vec3(1), 0.f, 0.f);

	gun_base.quad_render_handle = create_quad_render(transform_handle, create_color(59,74,94), gun_base_t::WIDTH, gun_base_t::HEIGHT, false, 0.f, -1);
	gun_base.rb_handle = create_rigidbody(transform_handle, false, gun_base_t::WIDTH, gun_base_t::HEIGHT, true, PHYSICS_RB_TYPE::NONE, true, true);
}

void gos_update() {

}