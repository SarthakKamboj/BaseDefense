#pragma once

#include <map>
#include <string>
#include <vector>

#include "gos.h"
#include "enemies.h"

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

    std::vector<base_t> gun_bases;
    std::vector<base_extension_t> attached_base_exts;
    std::vector<gun_t> attached_guns;
    std::vector<bullet_t> bullets;
    std::vector<enemy_bullet_t> enemy_bullets;
    std::vector<attachment_t> attachments;
    std::vector<enemy_t> enemies;
    std::vector<enemy_spawner_t> enemy_spawners;
    std::vector<int> enemies_to_delete;

    score_t score;
    float time_left = 20;
    bool paused = false;
};

void init_gos_globals();