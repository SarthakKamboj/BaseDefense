#pragma once

#include <map>
#include <string>

#define PREVIEW_SELECTOR_Z_POS_KEY "preview_selector"
#define PREVIEW_Z_POS_KEY "preview"
#define ATT_Z_POS_KEY "att"
#define BASE_Z_POS_KEY "base"
#define BASE_EXT_Z_POS_KEY "base_ext"
#define GUN_Z_POS_KEY "gun"
#define BULLET_Z_POS_KEY "bullet"
#define ENEMY_BULLET_Z_POS_KEY "enemy_bullet"
#define ENEMY_SPAWNER_Z_POS_KEY "enemy_spawner"
#define ENEMY_Z_POS_KEY "enemy"

struct go_globals_t {
    std::map<std::string, int> z_positions;
};

void init_gos_globals();