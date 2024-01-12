#include "bck.h"

#include "transform/transform.h"
#include "utils/io.h"
#include "gfx/gfx_data/texture.h"
#include "gfx/quad.h"
#include "constants.h"
#include "globals.h"

extern globals_t globals;

int parallax_bck::bck_texture = -1;
int parallax_bck::transform_handles[PARALLAX_BCK::NUM_BCKS] = {-1, -1, -1, -1};
int parallax_bck::rec_render_handles[PARALLAX_BCK::NUM_BCKS] = {-1, -1, -1, -1};

static int ground_height = -10;
void init_parallax_bck_data() {
	parallax_bck::transform_handles[EVEN1] = create_transform(glm::vec3(-1, -1, 0), glm::vec3(1), 0.f, 0.f);
	parallax_bck::transform_handles[EVEN2] = create_transform(glm::vec3(-1, -1, 0), glm::vec3(1), 0.f, 0.f);
	parallax_bck::transform_handles[ODD1] = create_transform(glm::vec3(-1, -1, 0), glm::vec3(1), 0.f, 0.f);
	parallax_bck::transform_handles[ODD2] = create_transform(glm::vec3(-1, -1, 0), glm::vec3(1), 0.f, 0.f);

	char resource_path[256]{};
	get_resources_folder_path(resource_path);
	char bck_texture_path[256]{};
	sprintf(bck_texture_path, "%s\\%s\\background\\background.png", resource_path, ART_FOLDER);
	parallax_bck::bck_texture = create_texture(bck_texture_path, 0);
	parallax_bck::rec_render_handles[EVEN1] = create_quad_render(parallax_bck::transform_handles[EVEN1], glm::vec3(1,0,0), -1, -1, false, 1.f, parallax_bck::bck_texture);
	parallax_bck::rec_render_handles[EVEN2] = create_quad_render(parallax_bck::transform_handles[EVEN2], glm::vec3(0,1,0), -1, -1, false, 1.f, parallax_bck::bck_texture);
	parallax_bck::rec_render_handles[ODD1] = create_quad_render(parallax_bck::transform_handles[ODD1], glm::vec3(0,0,1), -1, -1, false, 1.f, parallax_bck::bck_texture);
	parallax_bck::rec_render_handles[ODD2] = create_quad_render(parallax_bck::transform_handles[ODD2], glm::vec3(1,1,0), -1, -1, false, 1.f, parallax_bck::bck_texture);
}

void update_parallax_bcks() {

	transform_t* even_bck_1 = get_transform(parallax_bck::transform_handles[EVEN1]);
	transform_t* even_bck_2 = get_transform(parallax_bck::transform_handles[EVEN2]);
	transform_t* odd_bck_1 = get_transform(parallax_bck::transform_handles[ODD1]);
	transform_t* odd_bck_2 = get_transform(parallax_bck::transform_handles[ODD2]);

	texture_t* bck_tex = get_tex(parallax_bck::bck_texture);
	assert(bck_tex);

	float window_width = globals.window.window_width;
	float window_height = globals.window.window_height;

	float bck_width = window_height * static_cast<float>(bck_tex->width) / bck_tex->height;

	for (int i = 0; i < PARALLAX_BCK::NUM_BCKS; i++) {
		set_quad_width_height(parallax_bck::rec_render_handles[i], bck_width, window_height);
	}

	even_bck_1->position.y = ground_height + ((window_height-ground_height)/2);
	even_bck_2->position.y = ground_height + ((window_height-ground_height)/2);
	odd_bck_1->position.y = ground_height + ((window_height-ground_height)/2);
	odd_bck_2->position.y = ground_height + ((window_height-ground_height)/2);

	int cam_x = globals.camera.pos.x;
	int cam_center_x_screen = cam_x + (window_width / 2);
	int bck_tile = floor(cam_center_x_screen / bck_width);

	if (bck_tile % 2 == 0) {
		even_bck_1->position.x = bck_tile * bck_width + (bck_width/2);
		even_bck_2->position.x = (bck_tile+2) * bck_width + (bck_width/2);
	} else {
		even_bck_1->position.x = (bck_tile-1) * bck_width + (bck_width/2);
		even_bck_2->position.x = (bck_tile+1) * bck_width + (bck_width/2);
	}

	if (fabs(bck_tile % 2) == 1) {
		odd_bck_1->position.x = bck_tile * bck_width + (bck_width/2);
		odd_bck_2->position.x = (bck_tile+2) * bck_width + (bck_width/2);
	} else {
		odd_bck_1->position.x = (bck_tile - 1) * bck_width + (bck_width/2);
		odd_bck_2->position.x = (bck_tile + 1) * bck_width + (bck_width/2);
	}
}