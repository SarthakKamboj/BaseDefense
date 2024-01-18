#pragma once

enum PARALLAX_BCK {
	EVEN1 = 0,
	EVEN2,
	ODD1,
	ODD2,

	NUM_BCKS
};

struct parallax_bck {
	static int transform_handles[NUM_BCKS];
	static int rec_render_handles[NUM_BCKS];
	static int bck_texture;
};
void init_parallax_bck_data();
void update_parallax_bcks();