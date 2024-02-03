#include "gos_globals.h"

go_globals_t go_globals;

void init_gos_globals() {
    auto& z_positions = go_globals.z_positions;

    z_positions[PREVIEW_Z_POS_KEY] = 6;
    z_positions[ATT_Z_POS_KEY] = 5;
	z_positions[BASE_Z_POS_KEY] = 0;
	z_positions[BASE_EXT_Z_POS_KEY] = 3;
	z_positions[GUN_Z_POS_KEY] = 4;
	z_positions[BULLET_Z_POS_KEY] = 2;
	z_positions[ENEMY_BULLET_Z_POS_KEY] = 2;
	z_positions[ENEMY_SPAWNER_Z_POS_KEY] = 0;
	z_positions[ENEMY_Z_POS_KEY] = 1;
};